// ============================================================================
// CANNONE 2D - PROIETTILI, FARFALLE E CUORI IN MOVIMENTO
// Basato sul progetto dell'esercitazione 6. OpenGL 3.3 + GLFW + GLAD + GLM.
// Tutti gli oggetti sono Geometry nella stessa Scene.
// Animazioni: aggiornamento della Model Matrix, senza riscrivere i VBO.
// ============================================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

#include "Shader.h"
#include "ShapeGenerator.h"
#include "Player.h"
#include "Scene.h"
#include "Renderer.h"
#include "Input.h"
#include "Gui.h"
#include <../ImGui/imgui.h>

namespace
{
    constexpr float WORLD_HALF = 100.0f;
    constexpr float PROJECTILE_RADIUS = 2.0f; //Raggio della circonferenza che approssima la shape
    constexpr float PROJECTILE_SCALE = 1.5f;  //SCALA del proiettile
    constexpr float BUTTERFLY_OBJ_SCALE = 20.0f; // scala della mesh shape.obj, impostata nel main
    constexpr float BUTTERFLY_RADIUS = 7.0f; // approssimazione della sagoma
    constexpr float HEART_RADIUS = 7.0f; // approssimazione della sagoma
    constexpr int MAX_BUTTERFLIES = 15;
    constexpr int MAX_HEARTS = 15;
    constexpr int MAX_PROJECTILES = 30;
    constexpr float FIRE_COOLDOWN = 0.18f;
    const glm::vec2 BASE_POSITION(0.0f, -78.0f);

    struct Projectile
    {
        std::size_t instanceIndex = 0;
        glm::vec2 position{0.0f};
        glm::vec2 velocity{0.0f};
        bool active = false;
    };

    enum class TargetType { Butterfly, Heart };

    struct Target
    {
        std::size_t instanceIndex = 0;
        TargetType type = TargetType::Butterfly;
        glm::vec2 position{0.0f};
        glm::vec2 direction{1.0f, 0.0f};
        float directionTimer = 0.0f;
        bool alive = true; // false dopo un colpo: non ricompare cambiando gli slider
    };

    std::mt19937 randomEngine(2026); // seed fisso: esperimenti riproducibili

    float randomBetween(float lo, float hi)
    {
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(randomEngine);
    }

    glm::mat4 translate2D(const glm::vec2& position)
    {
        return glm::translate(glm::mat4(1.0f),
                              glm::vec3(position, 0.0f));
    }

    glm::mat4 rotateZ(float degrees)
    {
        return glm::rotate(glm::mat4(1.0f), glm::radians(degrees),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    }

    glm::mat4 scale2D(float sx, float sy)
    {
        return glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, 1.0f));
    }

    MeshData makeLink(float length, float thickness, const glm::vec4& color)
    {
        const float h = thickness * 0.5f;
        MeshData mesh;
        mesh.primitiveType = GL_TRIANGLES;
        mesh.positions = {
            {0.0f,-h,0.0f}, {length,-h,0.0f}, {length,h,0.0f},
            {0.0f,-h,0.0f}, {length,h,0.0f}, {0.0f,h,0.0f}
        };
        mesh.colors.assign(mesh.positions.size(), color);
        return mesh;
    }

    // Restituisce anche i limiti ortografici, necessari per convertire il mouse.
    void updateOrthoProjection(GLFWwindow* window, Renderer& renderer,
                               float& halfWidth, float& halfHeight)
    {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        if (w <= 0 || h <= 0) return;
        const float aspect = static_cast<float>(w) / static_cast<float>(h);
        halfWidth = WORLD_HALF;
        halfHeight = WORLD_HALF;
        if (aspect >= 1.0f) halfWidth *= aspect;
        else halfHeight /= aspect;
        renderer.setProjectionMatrix(glm::ortho(
            -halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f));
        glViewport(0, 0, w, h);
    }

    glm::vec2 cursorToWorld(GLFWwindow* window, float halfWidth, float halfHeight)
    {
        // Il cursore GLFW e' espresso in coordinate logiche di finestra.
        // Si usa glfwGetWindowSize (non la dimensione del framebuffer).
        double mouseX = 0.0, mouseY = 0.0;
        int w = 0, h = 0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        glfwGetWindowSize(window, &w, &h);
        if (w <= 0 || h <= 0) return glm::vec2(0.0f);
        const float nx = 2.0f * static_cast<float>(mouseX) / w - 1.0f;
        const float ny = 1.0f - 2.0f * static_cast<float>(mouseY) / h;
        return glm::vec2(nx * halfWidth, ny * halfHeight);
    }

    // Il raggio e' un'approssimazione conservativa della sagoma visibile.
    float targetRadius(const Target& t)
    {
        return t.type == TargetType::Butterfly ? BUTTERFLY_RADIUS : HEART_RADIUS;
    }

    // Test tra due cerchi in coordinate mondo, con piccolo margine aggiuntivo.
    bool overlapping(const Target& a, const Target& b, float margin = 1.5f)
    {
        const glm::vec2 d = a.position - b.position;
        const float minDistance = targetRadius(a) + targetRadius(b) + margin;
        return glm::dot(d, d) < minDistance * minDistance;
    }

    // Cerca una posizione che non si sovrapponga agli altri bersagli attivi.
    // La funzione viene usata sia all'avvio sia dopo un colpo.
    // In caso di alta densita' effettua piu' tentativi, poi usa una
    // ricerca su una griglia di posizioni candidate.
    void respawnTarget(Target& b, const std::vector<Target>& targets,
                       const Scene& scene)
    {
        const float radius = targetRadius(b);
        const float xmin = -88.0f + radius;
        const float xmax =  88.0f - radius;
        const float ymin =   0.0f + radius;
        const float ymax =  88.0f - radius;

        auto freePosition = [&](const glm::vec2& candidate) {
            Target probe = b;
            probe.position = candidate;
            for (const Target& other : targets) {
                if (&other == &b || other.instanceIndex == b.instanceIndex) continue;
                if (!other.alive || !scene.instance(other.instanceIndex).visible) continue;
                if (overlapping(probe, other)) return false;
            }
            return true;
        };

        bool found = false;
        for (int attempt = 0; attempt < 500; ++attempt) {
            glm::vec2 candidate(randomBetween(xmin, xmax), randomBetween(ymin, ymax));
            if (freePosition(candidate)) {
                b.position = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            // Ricerca deterministica di riserva, utile con molti bersagli.
            for (float y = ymin; y <= ymax && !found; y += 3.0f) {
                for (float x = xmin; x <= xmax; x += 3.0f) {
                    if (freePosition(glm::vec2(x, y))) {
                        b.position = glm::vec2(x, y);  
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            // Caso limite: spazio insufficiente per inserire un altro cerchio.
            // Evitiamo di introdurre una sovrapposizione: segnaliamo il problema.
            std::cerr << "Impossibile trovare una posizione libera per un bersaglio.\n";
        }
        const float angle = randomBetween(0.0f, 2.0f * glm::pi<float>());
        b.direction = glm::vec2(std::cos(angle), std::sin(angle));
        b.directionTimer = randomBetween(0.7f, 2.0f);
    }

    // Collisioni fra tutti i bersagli visibili (farfalle e cuori).
    // Risolve la compenetrazione e riflette le componenti normali delle
    // direzioni: urto elastico semplificato fra oggetti di massa uguale.
    
    void resolveTargetCollisions(
        Scene& scene,
        std::vector<Target>& targets)
    {
        // TODO 5B - COLLISIONI TRA BERSAGLI (approfondimento)
        // Confrontare tutte le coppie i<j di bersagli alive e visibili.
        // Se la distanza e' minore della somma dei raggi, separarli
        // lungo la congiungente e aggiornare le direzioni di movimento.
        // Aggiornare poi le Model Matrix delle istanze spostate.
        // Numero complessivo di bersagli presenti nel vettore.

        const std::size_t n = targets.size();

        // Esamina tutte le coppie di bersagli.
        // Utilizziamo i < j per evitare di confrontare
        // due volte la stessa coppia e un oggetto con se stesso.
        for (std::size_t i = 0; i < n; ++i)
        {
            Target& a = targets[i];

            // Un bersaglio eliminato non partecipa alle collisioni.
            if (!a.alive)
                continue;

            // Recupera l'istanza grafica del primo bersaglio.
            GeometryInstance& instanceA =
                scene.instance(a.instanceIndex);

            // Ignora le istanze nascoste dalla GUI.
            if (!instanceA.visible)
                continue;

            // Determina il raggio del primo bersaglio.
            float radiusA;

            if (a.type == TargetType::Butterfly)
                radiusA = BUTTERFLY_RADIUS;
            else
                radiusA = HEART_RADIUS;

            // Confronta il primo bersaglio con tutti quelli successivi.
            for (std::size_t j = i + 1; j < n; ++j)
            {
                Target& b = targets[j];

                // Ignora i bersagli eliminati.
                if (!b.alive)
                    continue;

                GeometryInstance& instanceB =
                    scene.instance(b.instanceIndex);

                // Ignora i bersagli non visualizzati.
                if (!instanceB.visible)
                    continue;

                // Determina il raggio del secondo bersaglio.
                float radiusB;

                if (b.type == TargetType::Butterfly)
                    radiusB = BUTTERFLY_RADIUS;
                else
                    radiusB = HEART_RADIUS;

                // Calcola il vettore dal centro di A al centro di B.
                const glm::vec2 difference =
                    b.position - a.position;

                // Calcola la distanza tra i centri.
                const float distance =
                    glm::length(difference);

                // Somma dei raggi dei cerchi di collisione.
                const float radiusSum =
                    radiusA + radiusB;

                // Se la distanza è maggiore o uguale alla somma
                // dei raggi, i due cerchi non si sovrappongono.
                if (distance >= radiusSum)
                    continue;

                // --------------------------------------------------
                // FASE 1: CALCOLO DELLA NORMALE DI COLLISIONE
                // --------------------------------------------------

                glm::vec2 normal;

                if (distance > 0.00001f)
                {
                    // Vettore unitario diretto da A verso B.
                    normal = difference / distance;
                }
                else
                {
                    // I centri coincidono: non possiamo dividere
                    // per zero. Scegliamo una direzione arbitraria.
                    normal = glm::vec2(1.0f, 0.0f);
                }

                // --------------------------------------------------
                // FASE 2: ELIMINAZIONE DELLA SOVRAPPOSIZIONE
                // --------------------------------------------------

                // Quantità di compenetrazione dei due cerchi.
                const float overlap =
                    radiusSum - distance;

                // Supponendo masse uguali, spostiamo
                // entrambi i bersagli di metà sovrapposizione.
                const glm::vec2 correction =
                    normal * (overlap * 0.5f);

                // A viene spostato nella direzione opposta
                // alla normale, B nella direzione della normale.
                a.position -= correction;
                b.position += correction;

                // --------------------------------------------------
                // FASE 3: RISPOSTA ALL'URTO
                // --------------------------------------------------

                // La direzione di ciascun bersaglio rappresenta
                // un vettore unitario. Supponiamo che i bersagli
                // si muovano con la stessa velocità scalare.

                const glm::vec2 relativeVelocity =
                    b.direction - a.direction;

                // Componente della velocità relativa
                // lungo la normale di collisione.
                const float normalSpeed =
                    glm::dot(relativeVelocity, normal);

                // Se normalSpeed < 0 i bersagli si avvicinano.
                // Se normalSpeed >= 0 si stanno già allontanando.
                if (normalSpeed < 0.0f)
                {
                    // Per due masse uguali e urto perfettamente
                    // elastico, le componenti normali delle
                    // velocità vengono scambiate.

                    a.direction += normalSpeed * normal;
                    b.direction -= normalSpeed * normal;

                    // Il progetto usa direction come vettore
                    // unitario e speed come velocità comune.
                    // Normalizziamo quindi le nuove direzioni.
                    //
                    // Nota: questa normalizzazione rende
                    // la risposta un'approssimazione dell'urto
                    // elastico, non una simulazione fisica esatta.

                    if (glm::length(a.direction) > 0.00001f)
                        a.direction = glm::normalize(a.direction);

                    if (glm::length(b.direction) > 0.00001f)
                        b.direction = glm::normalize(b.direction);
                }

                // --------------------------------------------------
                // FASE 4: AGGIORNAMENTO DELLE MODEL MATRIX
                // --------------------------------------------------

                // Determina le scale utilizzate per disegnare
                // le due geometrie condivise.
                float scaleA;
                float scaleB;

                if (a.type == TargetType::Butterfly)
                    scaleA = BUTTERFLY_OBJ_SCALE;
                else
                    scaleA = 8.0f;

                if (b.type == TargetType::Butterfly)
                    scaleB = BUTTERFLY_OBJ_SCALE;
                else
                    scaleB = 8.0f;

                // Aggiorna le trasformazioni delle istanze
                // in base alle nuove posizioni corrette.
                instanceA.modelMatrix =
                    translate2D(a.position) *
                    scale2D(scaleA, scaleA);

                instanceB.modelMatrix =
                    translate2D(b.position) *
                    scale2D(scaleB, scaleB);
            }
        }
    }
    void updateTargets(
        Scene& scene,
        std::vector<Target>& targets,
        int activeButterflies,
        int activeHearts,
        float speed,
        float dt)
    {
        // TODO 4 - MOVIMENTO DEI BERSAGLI
       // 1. Determinare quanti cuori e farfalle sono abilitati dalla GUI.
       // 2. Impostare instance.visible = enabled && target.alive.
       // 3. Per i soli bersagli visibili: aggiornare directionTimer,
       //    cambiare direzione a intervalli casuali, muovere con speed*dt,
       //    rimbalzare sui bordi e ricostruire la Model Matrix.
       // Per ora: al massimo 1 farfalla e 1 cuore fermi;
       // nel TODO 4 eliminare il limite std::min(..., 1).
       // Non riattivare i bersagli gia' colpiti.
        
       
        // Contatori delle farfalle e dei cuori
        // incontrati durante la visita del vettore.
        int butterfliesSeen = 0;
        int heartsSeen = 0;

        // Visita tutti i bersagli presenti nel vettore.
        for (Target& b : targets)
        {
            // Recupera dalla Scene l'istanza grafica associata al bersaglio corrente.
            GeometryInstance& instance = scene.instance(b.instanceIndex);

            // Indica se il bersaglio rientra nel numero di oggetti richiesto dall'interfaccia ImGui.
            bool enabled = false;
            //enabled risponde alla domanda «Questa farfalla rientra tra quelle richieste dalla GUI ? », 
            // mentre alive risponde alla domanda «Questa farfalla è ancora in gioco ? »
            // Solo se entrambe le risposte sono sì, la farfalla viene disegnata.
            if (b.type == TargetType::Butterfly)
            {
                // Abilita soltanto le prime  activeButterflies farfalle.
                enabled = butterfliesSeen < activeButterflies;

                // Conta la farfalla appena incontrata.
                butterfliesSeen++;
            }
            else
            {
                // Abilita soltanto i primi  activeHearts cuori.
                enabled =  heartsSeen < activeHearts;

                // Conta il cuore appena incontrato.
                heartsSeen++;
            }

            // L'istanza è visibile soltanto se:
            // - è abilitata dalla GUI;
            // - il bersaglio non è stato colpito.

            instance.visible = enabled && b.alive;

            // Se il bersaglio è invisibile,
            // passa direttamente al successivo.
            if (!instance.visible)
            {
                continue;
            }

            // Determina il fattore di scala
            // in base alla geometria utilizzata.
            float scale;

            if (b.type == TargetType::Butterfly)
            {
                scale = BUTTERFLY_OBJ_SCALE;
            }
            else
            {
                scale = 8.0f;
            }
            // 1. Aggiorna il timer per il cambio di direzione.
            b.directionTimer -= dt;

            // 2. Cambia casualmente la direzione quando scade.
            if (b.directionTimer <= 0.0f)
            {
                const float angle =
                    randomBetween(-0.8f, 0.8f);

                const float c = std::cos(angle);
                const float s = std::sin(angle);

                const glm::vec2 oldDirection = b.direction;

                b.direction = glm::normalize(
                    glm::vec2(
                        c * oldDirection.x - s * oldDirection.y,
                        s * oldDirection.x + c * oldDirection.y
                    )
                );

                b.directionTimer =
                    randomBetween(0.7f, 2.0f);
            }
            //-------------Per l'animazionr
            b.position += b.direction * speed * dt;


            //Se tocca i bordi 
            if (b.position.x < -88.0f)
            {
                b.position.x = -88.0f;
                b.direction.x = std::abs(b.direction.x);
            }
            else if (b.position.x > 88.0f)
            {
                b.position.x = 88.0f;
                b.direction.x = -std::abs(b.direction.x);
            }

            if (b.position.y < 0.0f)
            {
                b.position.y = 0.0f;
                b.direction.y = std::abs(b.direction.y);
            }
            else if (b.position.y > 88.0f)
            {
                b.position.y = 88.0f;
                b.direction.y = -std::abs(b.direction.y);
            }
            // Costruisce la Model Matrix:
            // prima scala la geometria locale,
            // poi la trasla nella posizione del mondo.
            //
            // M = T * S
            instance.modelMatrix =
                translate2D(b.position) *
                scale2D(scale, scale);
        }

       
    }
   
    void fireProjectile(std::vector<Projectile>& projectiles, const glm::vec2& origin,
                        const glm::vec2& direction, float speed)
    {
        // TODO 2 - SPARO
        
        for (Projectile& p : projectiles)
        {
            if (p.active) continue;
            // Cercare il primo Projectile con active == false.
            p.active = true;
            // Impostare active=true, position=origin e velocity=direction*speed.
            p.position = origin;
            p.velocity = direction * speed;
            return;
        }

      
    }

    void updateProjectiles(Scene& scene, std::vector<Projectile>& projectiles,
                           std::vector<Target>& targets,
                           float halfWidth, float halfHeight, float dt, int& score)
    {
        // TODO 3 - MOVIMENTO DEI PROIETTILI
        // Per ogni proiettile attivo: position += velocity * dt.
        // Disattivarlo quando esce dalla regione visibile.
        for (Projectile& p : projectiles) {
            if (!p.active) continue;
            p.position += p.velocity * dt;
            if (std::abs(p.position.x) > halfWidth + 5.0f ||
                std::abs(p.position.y) > halfHeight + 5.0f) {
                p.active = false;
                continue;
            }

        // TODO 5A - COLLISIONE PROIETTILE/BERSAGLIO
        // Per ogni proiettile attivo confrontare il centro con ciascun
        // bersaglio vivo e visibile usando:
        // dot(p.position-b.position, p.position-b.position)
        // <= (PROJECTILE_RADIUS + targetRadius(b))^2.
           
        // Al colpo: p.active=false; b.alive=false;
        // scene.instance(b.instanceIndex).visible=false; ++score;
        // NON richiamare respawnTarget(): il bersaglio resta eliminato.
            // Esamina tutti i bersagli presenti nel vettore targets.
            for (Target& b : targets)
            {
                // Recupera l'istanza grafica associata al bersaglio.
                GeometryInstance& instance =
                    scene.instance(b.instanceIndex);

                // Se il bersaglio è già stato colpito,
                // non deve partecipare al controllo delle collisioni.
                if (b.alive == false)
                {
                    continue;
                }

                // Se l'istanza non è visibile,
                // il bersaglio non partecipa alla collisione.
                if (instance.visible == false)
                {
                    continue;
                }

                // Determina il raggio di collisione
                // in base al tipo di bersaglio.
                float targetRadius;

                if (b.type == TargetType::Butterfly)
                {
                    targetRadius = BUTTERFLY_RADIUS;
                }
                else
                {
                    targetRadius = HEART_RADIUS;
                }

                // Calcola la somma dei raggi dei due oggetti.
                const float radiusSum =
                    PROJECTILE_RADIUS + targetRadius;

                // Calcola il vettore che congiunge
                // il centro del bersaglio al centro del proiettile.
                const glm::vec2 difference =
                    p.position - b.position;

                // Calcola il quadrato della distanza tra i centri.
                const float distanceSquared =
                    glm::dot(difference, difference);

                // Calcola il quadrato della somma dei raggi.
                const float radiusSumSquared =
                    radiusSum * radiusSum;

                // Verifica se i due cerchi si intersecano.
                if (distanceSquared <= radiusSumSquared)
                {
                    // Disattiva il proiettile che ha colpito il bersaglio.
                    p.active = false;

                    // Segna il bersaglio come eliminato dal gioco.
                    b.alive = false;

                    // Rende invisibile l'istanza grafica.
                    instance.visible = false;

                    // Incrementa il punteggio.
                    score = score + 1;

                    // Interrompe il ciclo sui bersagli:
                    // il proiettile ha già colpito un oggetto.
                    break;
                }
            }
        }

         
        for (const Projectile& p : projectiles) {
            GeometryInstance& instance = scene.instance(p.instanceIndex);
            instance.visible = p.active;
            if (p.active) instance.modelMatrix = translate2D(p.position)* scale2D(PROJECTILE_SCALE, PROJECTILE_SCALE);
        }
        
    }
}

// Crea un mirino RGBA di 32x32 pixel. L'hotspot e' il centro (16,16).
// Il disegno viene realizzato in memoria: non servono file immagine esterni.
GLFWcursor* createCrosshairCursor()
{
    constexpr int size = 32;
    unsigned char pixels[size * size * 4] = {};

    auto setPixel = [&](int x, int y, unsigned char r,
                        unsigned char g, unsigned char b) {
        if (x < 0 || x >= size || y < 0 || y >= size) return;
        const int i = (y * size + x) * 4;
        pixels[i] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = 255;
    };

    // Quattro bracci con contorno scuro per il contrasto su ogni sfondo.
    for (int v = 3; v <= 29; ++v) {
        if (v >= 13 && v <= 19) continue; // piccolo spazio al centro
        for (int offset = -1; offset <= 1; ++offset) {
            const unsigned char c = offset == 0 ? 255 : 20;
            setPixel(16 + offset, v, c, c, c);
            setPixel(v, 16 + offset, c, c, c);
        }
    }
    // Punto centrale.
    setPixel(16, 16, 255, 210, 50);

    GLFWimage image{};
    image.width = size;
    image.height = size;
    image.pixels = pixels;
    return glfwCreateCursor(&image, 16, 16);
}

int main()
{
    if (!glfwInit()) {
        std::cerr << "Errore inizializzazione GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1100, 850,
        "Cannone 2D - Farfalle e cuori", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Errore inizializzazione GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    //Gestione Interazioni
    glfwSetKeyCallback(window, Input::keyCallback);
    glfwSetCursorPosCallback(window, Input::cursorPositionCallback);
    glfwSetFramebufferSizeCallback(window, Input::framebufferSizeCallback);

    //Inizializzazione dell'interfaccia
    Gui gui;
    gui.initialize(window);
    // ImGui non cambia la forma del cursore: il mirino resta visibile.
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    GLFWcursor* crosshairCursor = createCrosshairCursor();
    if (crosshairCursor != nullptr)
        glfwSetCursor(window, crosshairCursor);
    Shader shader;
    if (!shader.load("shader/vertexshaderC.glsl", "shader/fragmentshaderC.glsl")) {
        std::cerr << "Errore caricamento shader: controllare working directory.\n";
        gui.shutdown();
        if (crosshairCursor != nullptr) {
            glfwSetCursor(window, nullptr);
            glfwDestroyCursor(crosshairCursor);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Scene scene;
    Renderer renderer(shader);
    float halfWidth = 100.0f, halfHeight = 100.0f;
     
    updateOrthoProjection(window, renderer, halfWidth, halfHeight);

    // Scelta della rappresentazione: modificare soltanto questa costante: OBJ, Primitives
    constexpr Player::GeometryMode PLAYER_GEOMETRY_MODE = Player::GeometryMode::OBJ;
    
    const std::string PLAYER_OBJ_FILENAME = "assets/fungo.obj";

    std::vector<Player::Part> playerParts;

    if (PLAYER_GEOMETRY_MODE == Player::GeometryMode::Primitives)
    {   // Colori
        const glm::vec4 barrelColor(0.23f, 0.67f, 0.95f, 1.0f);
        const glm::vec4 baseColor(0.98f, 0.78f, 0.25f, 1.0f);
        // Dimensioni del cannone
        constexpr float BARREL_LENGTH = 24.0f;
        constexpr float BARREL_THICKNESS = 6.0f;
        constexpr float BASE_RADIUS = 7.0f;

        // 1. Canna rettangolare
        playerParts.push_back({makeLink(BARREL_LENGTH,BARREL_THICKNESS,barrelColor),glm::mat4(1.0f)});
        // 2. Base circolare
        playerParts.push_back({ShapeGenerator::createCircle(0.0f, 0.0f,BASE_RADIUS, BASE_RADIUS,32,baseColor, baseColor),glm::mat4(1.0f)});
    }

    // La geometria viene fornita direttamente al costruttore.
    // Se la modalità è OBJ, costruisce il Player dal file.
    // Altrimenti, costruisce il Player assemblando le primitive.

    // Sceglie il costruttore appropriato senza modificare la classe Player.
    Player player = [&]() -> Player
    {
        if (PLAYER_GEOMETRY_MODE == Player::GeometryMode::OBJ)
            return Player(PLAYER_OBJ_FILENAME);
        else
            return Player(std::move(playerParts));
    }();

    const bool playerReady = player.initialize(scene);
    if (!playerReady) {
        std::cerr << "Impossibile inizializzare il player.\n";
        gui.shutdown();
        if (crosshairCursor != nullptr) {
            glfwSetCursor(window, nullptr);
            glfwDestroyCursor(crosshairCursor);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Una sola geometria  condivisa da tutte le istanze delle farfalle.
    //const MeshData butterflyMesh= ShapeGenerator::createButterfly(0, 0, 0.05f, 0.05f, 80,
    //    glm::vec4(0.0f, 0.85f, 0.85f, 1.0f), glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));

    //Leggere la geometria prodotta tramite modellatore con curve di Hermite
    MeshData butterflyMesh;
    std::string butterflyLoadError;
    std::string butterflyPath;
    for (const char* path : { "assets/shape.obj", "shape.obj" }) {
        if (PlayerShapeLoader::load(path, butterflyMesh, butterflyLoadError)) {
            butterflyPath = path;
            break;
        }
    }
    if (butterflyPath.empty()) {
        std::cerr << "ERRORE caricamento shape.obj: " << butterflyLoadError << '\n';
        gui.shutdown();
        if (crosshairCursor != nullptr) {
            glfwSetCursor(window, nullptr);
            glfwDestroyCursor(crosshairCursor);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    //Aggiungere la geomertia alla mesh --> Crea VAO (uno solo)
    const std::size_t butterflyGeometry = scene.addGeometry(butterflyMesh);

    // Una sola geometria  condivisa da tutte le istanze dei cuori.
    const MeshData heartMesh = ShapeGenerator::createHeart(0, 0, 0.05f, 0.05f, 80,
        glm::vec4(1.0f, 0.85f, 0.85f, 1.0f), glm::vec4(0.95f, 0.12f, 0.32f, 1.0f));
     const std::size_t heartGeometry = scene.addGeometry(heartMesh);

    std::vector<Target> targets;
    // Prenota memoria sufficiente per tutte le farfalle e i cuori
    targets.reserve(MAX_BUTTERFLIES + MAX_HEARTS);
    
    for (int i = 0; i < MAX_BUTTERFLIES; ++i) {
        // Crea un oggetto Target 
        Target b;
        //che rappresenta una farfalla nella logica del gioco.
        b.type = TargetType::Butterfly;
        //Crea un'istanza grafica nella scena: tutte le istanze condividono la stessa geometria
        //addInstance() restituisce l'indice dell'istanza.
        b.instanceIndex = scene.addInstance(butterflyGeometry);
        // Inserisce una copia del bersaglio nel vettore.
        // Il campo instanceIndex collega il Target
        // alla corrispondente istanza grafica.
        targets.push_back(b);
        // L'istanza viene inizialmente nascosta.
        // Sara' resa visibile quando la farfalla
        // dovra' comparire durante il gioco.
        scene.instance(b.instanceIndex).visible = false;
    }

    //Adesso in targets vengono aggiunti anche i cuori
    for (int i = 0; i < MAX_HEARTS; ++i) {
        Target h;
        h.type = TargetType::Heart;
        h.instanceIndex = scene.addInstance(heartGeometry);
        targets.push_back(h);
        scene.instance(h.instanceIndex).visible = false;
    }

    // Posizionamento iniziale senza sovrapposizioni.
    // Solo gli oggetti gia' posizionati sono considerati dal test.
    for (Target& t : targets) {
        // ogni target di dell'insieme dei targets viene passato per riferimento 
        respawnTarget(t, targets, scene);
        //viene modificato direttamente il campo position dell'elemento corrente del vettore targets
        scene.instance(t.instanceIndex).visible = true;
    }

    
    //Definisco la matrice di modellazione per ogni target: farfalla o cuore della struttura targets:
    int initializedButterflies = 0, initializedHearts = 0;
    for (Target& b : targets) {
         if (b.type == TargetType::Butterfly && initializedButterflies++ == 0)
            b.position = glm::vec2(-38.0f, 45.0f);
        if (b.type == TargetType::Heart && initializedHearts++ == 0)
            b.position = glm::vec2(38.0f, 45.0f);
        float sc;
        if (b.type == TargetType::Butterfly)
           // Il bersaglio è una farfalla
            sc = BUTTERFLY_OBJ_SCALE;
        else
           // Il bersaglio è un cuore
            sc = 8.0f;
        scene.instance(b.instanceIndex).modelMatrix = translate2D(b.position) * scale2D(sc, sc);
    }

    const MeshData bulletMesh = ShapeGenerator::createCircle(0, 0,
        PROJECTILE_RADIUS, PROJECTILE_RADIUS, 24, glm::vec4(1, 1, 0.65f, 1), glm::vec4(1, 0.65f, 0.10f, 1));
    
    // Fattori di scala del proiettile
    const glm::vec3 bulletScale(PROJECTILE_SCALE, PROJECTILE_SCALE, 1.0f);
    const std::size_t projectileGeometry = scene.addGeometry(bulletMesh);
    std::vector<Projectile> projectiles(MAX_PROJECTILES);
    for (Projectile& p : projectiles) {
        p.instanceIndex = scene.addInstance(projectileGeometry);

        // Matrice di scala iniziale
        glm::mat4 model(1.0f);
        model = glm::scale(model, bulletScale);

        scene.instance(p.instanceIndex).modelMatrix = model;
        scene.instance(p.instanceIndex).visible = false;

    }

    // Si lascia il controllo dell'input alla finestra, senza sparare dalla GUI.
    bool previousFire = false;
    float fireCooldown = 0.0f;
    double lastTime = glfwGetTime();
    int score = 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = std::min(0.05f, std::max(0.0f, static_cast<float>(now - lastTime)));
        lastTime = now;
        updateOrthoProjection(window, renderer, halfWidth, halfHeight);

        gui.beginFrame();
        gui.draw();
        if (gui.resetRequested())
        {   // Azzera il punteggio.
            score = 0;
            // Conta separatamente farfalle e cuori.
            int butterflyCount = 0;
            int heartCount = 0;
            // Ripristina tutti i bersagli.
            for (Target& b : targets)
            {
                // Il bersaglio torna in gioco.
                b.alive = true;
                float scale;
                if (b.type == TargetType::Butterfly)
                {
                    // Posizione iniziale della prima farfalla.
                    if (butterflyCount == 0)
                    {
                        b.position = glm::vec2(-38.0f, 45.0f);
                    }

                    butterflyCount++;
                    scale = BUTTERFLY_OBJ_SCALE;
                }
                else
                {
                    // Posizione iniziale del primo cuore.
                    if (heartCount == 0)
                    {
                        b.position = glm::vec2(38.0f, 45.0f);
                    }

                    heartCount++;
                    scale = 8.0f;
                }

                // Aggiorna la matrice di modellazione.
                GeometryInstance& instance =
                    scene.instance(b.instanceIndex);

                instance.modelMatrix =
                    translate2D(b.position) *
                    scale2D(scale, scale);

                // La visibilità definitiva viene stabilita
                // da updateTargets(), in base alla GUI.
                instance.visible = true;
            }

            // Disattiva tutti i proiettili.
            for (Projectile& p : projectiles)
            {
                p.active = false;

                scene.instance(p.instanceIndex).visible = false;
            }

            // Azzera il tempo di attesa tra gli spari.
            fireCooldown = 0.0f;
        }

        // TODO 1 - PUNTAMENTO DEL CANNONE
        // Convertire il mouse in coordinate del mondo con cursorToWorld().
        const glm::vec2 target = cursorToWorld(window, halfWidth, halfHeight);
        // Calcolare aim = target - BASE_POSITION e normalizzarlo.
        const glm::vec2 aim = target - BASE_POSITION;
        const glm::vec2 direction = glm::normalize(aim);
        // Calcolare angleDeg = degrees(atan2(direction.y, direction.x)).
        const float angleDeg = glm::degrees(std::atan2(direction.y, direction.x));
        // Trasformazione globale: posizione nel mondo e puntamento verso il mouse.
        const glm::mat4 Tbase = glm::translate(
            glm::mat4(1.0f), glm::vec3(BASE_POSITION, 0.0f));
        const glm::mat4 Raim = glm::rotate(
            glm::mat4(1.0f), glm::radians(angleDeg),
            glm::vec3(0.0f, 0.0f, 1.0f));

        // Entrambe le rappresentazioni hanno una trasformazione locale.
        // Cambiano soltanto i parametri, non la formula della Model Matrix.
        glm::vec3 localTranslation(0.0f, 0.0f, 0.0f);
        glm::vec3 localScale(1.0f, 1.0f, 1.0f);
        float localRotationDegrees = 0.0f;
        glm::vec3 muzzleLocal(24.0f, 0.0f, 0.0f);

        if (PLAYER_GEOMETRY_MODE == Player::GeometryMode::OBJ)
        {
            // Adattamento della mesh importata.
            localTranslation = glm::vec3(12.0f, 0.0f, 0.0f);
            localScale = glm::vec3(50.f, 50.2836f, 1.0f);
            localRotationDegrees = -90.0f;
            // Punto di emissione definito nel riferimento originale dell'OBJ.
            muzzleLocal = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        else
        {
            // Cannone assemblato nel proprio riferimento locale.
            // La canna va da (0,0) a (24,0).
            // Questi valori possono essere modificati indipendentemente.
            localTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
            localScale = glm::vec3(1.0f, 1.0f, 1.0f);
            localRotationDegrees = 0.0f;
            muzzleLocal = glm::vec3(24.0f, 0.0f, 0.0f);
        }

         
        // Mlocal = Tlocal * Rlocal * Slocal
        glm::mat4 localModel(1.0f);
        localModel = glm::translate(localModel, localTranslation);
        localModel = glm::rotate(localModel,
            glm::radians(localRotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
        localModel = glm::scale(localModel, localScale);

        // Stessa composizione per OBJ e assemblaggio di primitive.
        const glm::mat4 playerModel = Tbase * Raim * localModel;
        player.setModelMatrix(scene, playerModel);

        // Il punto di sparo segue la stessa trasformazione della geometria.
        const glm::vec4 muzzleWorld4 =
            playerModel * glm::vec4(muzzleLocal, 1.0f);
        const glm::vec2 muzzle(muzzleWorld4.x, muzzleWorld4.y);

        const ImGuiIO& io = ImGui::GetIO();
        const bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        const bool firePressed = (mousePressed && !io.WantCaptureMouse) ||
                                 (spacePressed && !io.WantCaptureKeyboard);
        fireCooldown = std::max(0.0f, fireCooldown - dt);
        if (firePressed && !previousFire && fireCooldown <= 0.0f) {
            fireProjectile(projectiles, muzzle, direction, gui.bulletSpeed());
            fireCooldown = FIRE_COOLDOWN;
        }
        previousFire = firePressed;

        updateTargets(scene, targets, gui.butterflyCount(), gui.heartCount(),
                      gui.butterflySpeed(), dt);
        resolveTargetCollisions(scene, targets);
        updateProjectiles(scene, projectiles, targets,
                          halfWidth, halfHeight, dt, score);
        int active = 0;
        for (const Projectile& p : projectiles) if (p.active) ++active;
        gui.setScore(score);
        gui.setActiveProjectiles(active);

        renderer.beginFrame(gui.clearColor());
        renderer.setWireframe(gui.wireframe());
        renderer.drawScene(scene);
        renderer.setWireframe(false);
        gui.render();
        glfwSwapBuffers(window);
    }

    scene.clear();
    shader.destroy();
    gui.shutdown();
    if (crosshairCursor != nullptr) {
        glfwSetCursor(window, nullptr);
        glfwDestroyCursor(crosshairCursor);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
