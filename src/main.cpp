#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

unsigned int loadCubemap(vector<std::string> faces);

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void renderQuad();

unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool bloom= false;
float exposure= 1.0f;
bool bloomKeyPressed = false;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLightMedved;
    PointLight pointLightMedved1;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);
glm::vec3 positionKonj;
int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendShader("resources/shaders/2.model_lighting.vs", "resources/shaders/blending.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader shaderBloom("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");




    // load models
    // -----------
    Model konjModel("resources/objects/HorseArmor/HorseArmor.obj");
    konjModel.SetShaderTextureNamePrefix("material.");

    Model medvedModel("resources/objects/BearSaddle/BearSaddle.obj");
    medvedModel.SetShaderTextureNamePrefix("material.");

    Model pticaModel("resources/objects/gull/GULL.OBJ");
    pticaModel.SetShaderTextureNamePrefix("material.");

    Model pecurkaModel("resources/objects/Pecurka/10192_MushroomShitake_v1-L3.obj");
    pecurkaModel.SetShaderTextureNamePrefix("material.");


    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }



    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    programState->pointLightMedved.position = glm::vec3(10.0f, 2.0f, -3.0f);
    programState->pointLightMedved.ambient = glm::vec3(3.5f, 3.5f, 3.5f);
    programState->pointLightMedved.diffuse = glm::vec3(2.0f, 2.0f, 2.0f);
    programState->pointLightMedved.specular = glm::vec3(5.0f, 5.0f, 5.0f);
    programState->pointLightMedved.constant = 1.0f;
    programState->pointLightMedved.linear = 0.09f;
    programState->pointLightMedved.quadratic = 0.032f;




    ourShader.use();
    ourShader.setInt("diffuseTexture", 0);
    blendShader.use();
    blendShader.setInt("diffuseTexture", 0);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    shaderBloom.use();
    shaderBloom.setInt("scene", 0);
    shaderBloom.setInt("bloomBlur", 1);






    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // cube VAO

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //load textures
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/meadow_rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_lf.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox/meadow_bk.jpg")
            };

    unsigned int cubemapTexture = loadCubemap(faces);


    ourShader.use();
    ourShader.setInt("skybox", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);




    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        pointLight.position = programState->backpackPosition;

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //Face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CW);

        //blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        //--------------------------------------MODEL KONJA-----------------------------------------------------
        glm::mat4 model1= glm::mat4(1.0f);

      // Primeniti rotaciju modela oko određene ose (na primer, oko y-ose za 90 stepeni)
        float angle = glm::radians(45.0f); // Ugao rotacije, možemo promeniti ovu vrednost po potrebi
        glm::vec3 axis(0.0f, 1.0f, 0.0f); // Osa rotacije, ovde smo odabrali y-osu
        model1 = glm::rotate(model1, angle, axis);
        // Može se  promeniti ova vrednost u zavisnosti od toga koliko želimo da pomerimo model
        model1 = glm::translate(model1, glm::vec3 (1.0f, -1.1f, 0.0f));
        model1 = glm::translate(model1, positionKonj);

        model1 = glm::scale(model1, glm::vec3(programState->backpackScale));

        ourShader.setMat4("model", model1);
        konjModel.Draw(ourShader);
        //=======================================================================================================

        ourShader.use();
        ourShader.setVec3("pointLight.position", programState->pointLightMedved.position);
        ourShader.setVec3("pointLight.ambient", programState->pointLightMedved.ambient);
        ourShader.setVec3("pointLight.diffuse", programState->pointLightMedved.diffuse);
        ourShader.setVec3("pointLight.specular", programState->pointLightMedved.specular);
        ourShader.setFloat("pointLight.constant", programState->pointLightMedved.constant);
        ourShader.setFloat("pointLight.linear", programState->pointLightMedved.linear);
        ourShader.setFloat("pointLight.quadratic", programState->pointLightMedved.quadratic);


//-------------------------------------------MODEL KONJA2---------------------------------------------------------
        glm::mat4 model9= glm::mat4(1.0f);

        // Primeniti rotaciju modela oko određene ose (na primer, oko y-ose za 90 stepeni)
        float angle9 = glm::radians(198.0f); // Ugao rotacije, možemo promeniti ovu vrednost po potrebi
        glm::vec3 axis9(0.0f, 1.0f, 0.0f); // Osa rotacije, ovde smo odabrali y-osu
        model9 = glm::rotate(model9, angle9, axis9);
        // Može se  promeniti ova vrednost u zavisnosti od toga koliko želimo da pomerimo model
        model9 = glm::translate(model9, glm::vec3 (-3.0f, -0.83f, -10.1f));


        model9 = glm::scale(model9, glm::vec3(0.82, 0.82, 0.82));

        ourShader.setMat4("model", model9);
        konjModel.Draw(ourShader);

        //=========================================================================================================

        //---------------------------------------MODELI PTICA------------------------------------------------------

            glm::mat4 model3 = glm::mat4(1.0f);

// Definisanje parametra za putanju ptice u obliku znaka beskonacnosti
            float loopRadius = 0.8f;   // Poluprecnik krivine
            float loopHeight = 0.5f;   // Visina leta
            float loopSpeed = 0.5f;    // Brzina letenja

// Izracunavanje pozicije ptice na osnovu vremena
            float time = glfwGetTime();
            float x = loopRadius * sin(loopSpeed * time);
            float y = loopHeight * cos(4 * loopSpeed * time);
            float z = loopRadius * sin(4 * loopSpeed * time);


            model3 = glm::scale(model3, glm::vec3(3.0f));
// Postavnjanje pozicije ptice na izracunate koordinate

            model3 = glm::translate(model3, glm::vec3(x, y, z));
            model3 = glm::translate(model3, glm::vec3(2, 1.2, 2));

// Rotiranje ptice tako da gleda u pravcu leta (ka centru znaka beskonacnosti)
            glm::vec3 direction(-x, -y, -z);
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            glm::mat4 rotation = glm::lookAt(glm::vec3(0.0f), direction, up);
            model3 *= rotation;


            ourShader.setMat4("model", model3);
            pticaModel.Draw(ourShader);

//---------------------------------------------- 2 MODEL PTICE -------------------------------------------------------------
        glm::mat4 model4 = glm::mat4(1.0f);

        float loopRadius1 = 0.9f;
        float loopHeight1 = 0.4f;
        float loopSpeed1 = 0.6f;

        float time1 = glfwGetTime();
        float x1 = loopRadius * sin(loopSpeed * time);
        float y1 = loopHeight * cos(3 * loopSpeed * time);
        float z1 = loopRadius * sin(5 * loopSpeed * time);

        model4 = glm::scale(model4, glm::vec3(3.0f));

        model4 = glm::translate(model4, glm::vec3(x1, y1, z1));
        model4 = glm::translate(model4, glm::vec3(3, 1.2, 2));

        glm::vec3 direction1(-x1, -y1, -z1);
        glm::vec3 up1(0.0f, 1.0f, 0.0f);
        glm::mat4 rotation1 = glm::lookAt(glm::vec3(0.0f), direction, up);
        model4 *= rotation;

        ourShader.setMat4("model", model4);
        pticaModel.Draw(ourShader);

        //------------------------------------- 3 MODEL PTICE ----------------------------------------------------
        glm::mat4 model5 = glm::mat4(1.0f);

        float loopRadius5 = 0.9f;
        float loopHeight5 = 0.4f;
        float loopSpeed5 = 0.6f;

        float time2 = glfwGetTime();
        float x2 = loopRadius * sin(loopSpeed * time);
        float y2 = loopHeight * cos(8 * loopSpeed * time);
        float z2 = loopRadius * sin(6 * loopSpeed * time);

        model5 = glm::scale(model5, glm::vec3(3.0f));

        model5 = glm::translate(model5, glm::vec3(x2, y2, z2));
        model5 = glm::translate(model5, glm::vec3(3, 1.2, 2));

        glm::vec3 direction2(-x2, -y2, -z2);
        glm::vec3 up2(0.0f, 1.0f, 0.0f);
        glm::mat4 rotation2 = glm::lookAt(glm::vec3(0.0f), direction, up);
        model5 *= rotation;

        ourShader.setMat4("model", model5);
        pticaModel.Draw(ourShader);
//=================================================================================================================

//------------------------------------------MODEL PECURKE---------------------------------------------------------

        glm::mat4 model6= glm::mat4(1.0f);

        // Primeniti rotaciju modela oko određene ose (na primer, oko y-ose za 90 stepeni)
        float angle6 = glm::radians(270.0f); // Ugao rotacije, možemo promeniti ovu vrednost po potrebi
        glm::vec3 axis6(1.0f, 0.0f, 0.0f); // Osa rotacije, ovde smo odabrali y-osu
        model6 = glm::rotate(model6, angle6, axis6);
        // MožE se  promeniti ova vrednost u zavisnosti od toga koliko želimo da pomerimo model
        model6 = glm::translate(model6, glm::vec3 ( 2.0f, -10.0f, -2.0f));


        model6 = glm::scale(model6, glm::vec3(0.2,0.2,0.2));

        ourShader.setMat4("model", model6);
        pecurkaModel.Draw(ourShader);

        //--------------------------------------------------------------------------------------------------
        glm::mat4 model7= glm::mat4(1.0f);


        float angle7 = glm::radians(270.0f);
        glm::vec3 axis7(0.0f, 1.0f, 0.0f);
        model7 = glm::rotate(model7, angle7, axis7);
        model7 = glm::rotate(model7, glm::radians(270.0f), glm::vec3 (1.0, 0.0, 0.0));

        model7 = glm::translate(model7, glm::vec3 ( 6.8f, 4.0f, -3.0f));


        model7 = glm::scale(model7, glm::vec3(0.4,0.4,0.4));

        ourShader.setMat4("model", model7);
        pecurkaModel.Draw(ourShader);

        //----------------------------------------------------------------------------------------------------
        glm::mat4 model8= glm::mat4(1.0f);


        float angle8 = glm::radians(270.0f);
        glm::vec3 axis8(0.0f, 1.0f, 0.0f);
        model7 = glm::rotate(model7, angle7, axis7);
        model7 = glm::rotate(model7, glm::radians(90.0f), glm::vec3 (0.0, 1.0, 0.0));

        model7 = glm::translate(model7, glm::vec3 ( 0.2f, 4.0f, -3.6f));


        model7 = glm::scale(model7, glm::vec3(0.9,0.9,0.9));

        ourShader.setMat4("model", model7);
        pecurkaModel.Draw(ourShader);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

//==================================================================================================================

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        bool horizontal = true, first_iteration = true;
        unsigned int amount = 5;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        //____________________________________________________________________________________________________
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderBloom.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderBloom.setInt("bloom", bloom);
        shaderBloom.setFloat("exposure", exposure);
        renderQuad();



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteVertexArrays(1, &skyboxVAO);

    glDeleteBuffers(1, &skyboxVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {



    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);


    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        positionKonj.z-= 0.2;
        // Pomeri model ulevo

    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Pomeri model udesno
        positionKonj.z +=0.2;

    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.09f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        exposure += 0.09f;
    }

    float maxBackwardDistance = -2.0f; // Prilagodite prema potrebi
    float maxForwardDistance = 8.0f;   // Prilagodite prema potrebi
    positionKonj.z = glm::clamp(positionKonj.z, maxBackwardDistance, maxForwardDistance);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

    }


}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}