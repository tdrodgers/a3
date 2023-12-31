#include "A3Engine.h"

#include <CSCI441/objects.hpp>

//*************************************************************************************
//
// Helper Functions

#ifndef M_PI
#define M_PI 3.14159265f
#endif

//*************************************************************************************
//
// Public Interface

A3Engine::A3Engine()
         : CSCI441::OpenGLEngine(4, 1,
                                 640, 480,
                                 "A3: The Cabin In The Woods") {

    for(auto& _key : _keys) _key = GL_FALSE;

    // Initialize some values for idle animation and also hero position to keep track of hero.
    heroPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    _yOffset = 0.1;
    _timeVariable = 0.0;
    _hoverAmount = 0.0;
    _mousePosition = glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED );
    _leftMouseButtonState = GLFW_RELEASE;
}

A3Engine::~A3Engine() {
    delete _pArcballCam;
}

void A3Engine::handleKeyEvent(GLint key, GLint action) {
    if(key != GLFW_KEY_UNKNOWN)
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));

    if(action == GLFW_PRESS ) {
        switch( key ) {
            // quit!
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;

            default: break; // suppress CLion warning
        }
    }
}

void A3Engine::handleMouseButtonEvent(GLint button, GLint action) {
    // if the event is for the left mouse button
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        // update the left mouse button's state
        _leftMouseButtonState = action;
    }
}

void A3Engine::handleCursorPositionEvent(glm::vec2 currMousePosition) {
    // if mouse hasn't moved in the window, prevent camera from flipping out
    if(_mousePosition.x == MOUSE_UNINITIALIZED) {
        _mousePosition = currMousePosition;
    }

    // Creates the zoom in and out feature for the ArcballCam.
    if(_leftMouseButtonState == GLFW_PRESS && _keys[GLFW_KEY_LEFT_SHIFT] || _keys[GLFW_KEY_RIGHT_SHIFT] ) {
        if(currMousePosition.y > _mousePosition.y) {
            _pArcballCam->moveForward(_cameraSpeed.x);
        }
        if(currMousePosition.y < _mousePosition.y) {
            _pArcballCam->moveBackward(_cameraSpeed.x);
        }
    }

    // if the left mouse button is being held down while the mouse is moving
    if(_leftMouseButtonState == GLFW_PRESS) {
        // rotate the camera by the distance the mouse moved
        _pArcballCam->rotate((currMousePosition.x - _mousePosition.x) * 0.005f,
                         (_mousePosition.y - currMousePosition.y) * 0.005f );
    }

    // update the mouse position
    _mousePosition = currMousePosition;
}

//*************************************************************************************
//
// Engine Setup

void A3Engine::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

    // set our callbacks
    glfwSetKeyCallback(mpWindow, lab05_engine_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, lab05_engine_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, lab05_engine_cursor_callback);
}

void A3Engine::mSetupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                        // enable depth testing
    glDepthFunc( GL_LESS );							                // use less than depth test

    glEnable(GL_BLEND);									            // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	    // use one minus blending equation

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	        // clear the frame buffer to black
}

void A3Engine::mSetupShaders() {
    _lightingShaderProgram = new CSCI441::ShaderProgram("shaders/A3.v.glsl", "shaders/A3.f.glsl" );
    _lightingShaderUniformLocations.mvpMatrix      = _lightingShaderProgram->getUniformLocation("mvpMatrix");
    _lightingShaderUniformLocations.materialColor  = _lightingShaderProgram->getUniformLocation("materialColor");
    // TODO #3A: assign uniforms
    _lightingShaderUniformLocations.normalMatrix = _lightingShaderProgram->getUniformLocation("normalMatrix");
    _lightingShaderUniformLocations.lightDirection = _lightingShaderProgram->getUniformLocation("lightDirection");
    _lightingShaderUniformLocations.lightColor = _lightingShaderProgram->getUniformLocation("lightColor");

    _lightingShaderAttributeLocations.vPos         = _lightingShaderProgram->getAttributeLocation("vPos");
    // TODO #3B: assign attributes
    _lightingShaderAttributeLocations.vertexNormal = _lightingShaderProgram->getAttributeLocation("vertexNormal");

}

void A3Engine::mSetupBuffers() {
    // TODO #4: need to connect our 3D Object Library to our shader
    CSCI441::setVertexAttributeLocations( _lightingShaderAttributeLocations.vPos, _lightingShaderAttributeLocations.vertexNormal );

    // TODO #5: give the hero the normal matrix location
    _pHero = new Hero(_lightingShaderProgram->getShaderProgramHandle(),
                      _lightingShaderUniformLocations.mvpMatrix,
                      _lightingShaderUniformLocations.normalMatrix,
                      _lightingShaderUniformLocations.materialColor);

    _createGroundBuffers();
    _generateEnvironment();
}

void A3Engine::_createGroundBuffers() {
    // TODO #8: expand our struct
    struct Vertex {
        GLfloat x, y, z;
        GLfloat nx, ny, nz;
    };

    // TODO #9: add normal data
    Vertex groundQuad[4] = {
            {-1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f},
            { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f},
            {-1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f},
            { 1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f}
    };

    GLushort indices[4] = {0,1,2,3};

    _numGroundPoints = 4;

    glGenVertexArrays(1, &_groundVAO);
    glBindVertexArray(_groundVAO);

    GLuint vbods[2];       // 0 - VBO, 1 - IBO
    glGenBuffers(2, vbods);
    glBindBuffer(GL_ARRAY_BUFFER, vbods[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundQuad), groundQuad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vPos);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)nullptr);

    // TODO #10: hook up vertex normal attribute
    glEnableVertexAttribArray(_lightingShaderAttributeLocations.vertexNormal);
    glVertexAttribPointer(_lightingShaderAttributeLocations.vertexNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbods[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void A3Engine::_generateEnvironment() {
    //******************************************************************
    // parameters to make up our grid size and spacing, feel free to
    // play around with this
    const GLfloat GRID_WIDTH = 100.0f;
    const GLfloat GRID_LENGTH = 100.0f;
    const GLfloat GRID_SPACING_WIDTH = 1.0f;
    const GLfloat GRID_SPACING_LENGTH = 1.0f;
    // precomputed parameters based on above
    const GLfloat LEFT_END_POINT = -GRID_WIDTH / 2.0f - 5.0f;
    const GLfloat RIGHT_END_POINT = GRID_WIDTH / 2.0f + 5.0f;
    const GLfloat BOTTOM_END_POINT = -GRID_LENGTH / 2.0f - 5.0f;
    const GLfloat TOP_END_POINT = GRID_LENGTH / 2.0f + 5.0f;
    //******************************************************************

    srand( time(0) );                                                   // seed our RNG

    // psych! everything's on a grid.
    for(int i = LEFT_END_POINT; i < RIGHT_END_POINT; i += GRID_SPACING_WIDTH) {
        for(int j = BOTTOM_END_POINT; j < TOP_END_POINT; j += GRID_SPACING_LENGTH) {
            // don't just draw a tiles ANYWHERE.
            if( i % 2 && j % 2 ) {
                // translate to spot
                glm::mat4 transToSpotMtx = glm::translate( glm::mat4(1.0), glm::vec3(i, 0.0f, j) );

                // compute height
                GLdouble height = 0.3f;
                // scale to tile size
                glm::mat4 scaleToHeightMtx = glm::scale( glm::mat4(1.0), glm::vec3(1, height, 1) );

                // translate up to grid
                glm::mat4 transToHeight = glm::translate( glm::mat4(1.0), glm::vec3(0, height/2.0f, 0) );

                // compute full model matrix
                glm::mat4 modelMatrix = transToHeight * scaleToHeightMtx * transToSpotMtx;

                // compute color
                glm::vec3 color( 0.4f, 0.4f, 0.4f );
                // store tile properties
                TileData currentTile = {modelMatrix, color};
                _tiles.emplace_back(currentTile );
            }
        }
    }
}

void A3Engine::mSetupScene() {
    _pArcballCam = new CSCI441::ArcballCam();
    _pArcballCam->setRadius(25.0f);
    _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) );
    _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
    _pArcballCam->setTheta(-M_PI / 7.0f );
    _pArcballCam->setPhi(M_PI / 1.2f );
    _pArcballCam->recomputeOrientation();
    _cameraSpeed = glm::vec2(0.25f, 0.02f);

    // TODO #6: set lighting uniforms
    glm::vec3 lightDirection(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor(1.0f,1.0f,1.0f);
    glProgramUniform3fv(_lightingShaderProgram->getShaderProgramHandle(), _lightingShaderUniformLocations.lightDirection, 1, &lightDirection[0]);
    glProgramUniform3fv(_lightingShaderProgram->getShaderProgramHandle(), _lightingShaderUniformLocations.lightColor, 1, &lightColor[0]);
}

//*************************************************************************************
//
// Engine Cleanup

void A3Engine::mCleanupShaders() {
    fprintf( stdout, "[INFO]: ...deleting Shaders.\n" );
    delete _lightingShaderProgram;
}

void A3Engine::mCleanupBuffers() {
    fprintf( stdout, "[INFO]: ...deleting VAOs....\n" );
    CSCI441::deleteObjectVAOs();
    glDeleteVertexArrays( 1, &_groundVAO );

    fprintf( stdout, "[INFO]: ...deleting VBOs....\n" );
    CSCI441::deleteObjectVBOs();

    fprintf( stdout, "[INFO]: ...deleting models..\n" );
    delete _pHero;
}

//*************************************************************************************
//
// Rendering / Drawing Functions - this is where the magic happens!

void A3Engine::_renderScene(glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // use our lighting shader program
    _lightingShaderProgram->useProgram();

    //// BEGIN DRAWING THE GROUND PLANE ////
    // draw the ground plane
    glm::mat4 groundModelMtx = glm::scale( glm::mat4(1.0f), glm::vec3(WORLD_SIZE, 1.0f, WORLD_SIZE));
    _computeAndSendMatrixUniforms(groundModelMtx, viewMtx, projMtx);

    glm::vec3 groundColor(0.9f, 0.9f, 0.9f);
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialColor, groundColor);

    glBindVertexArray(_groundVAO);
    glDrawElements(GL_TRIANGLE_STRIP, _numGroundPoints, GL_UNSIGNED_SHORT, (void*)0);
    //// END DRAWING THE GROUND PLANE ////

    //// BEGIN DRAWING THE TILES ////
    for( const TileData& currentTile : _tiles ) {
        _computeAndSendMatrixUniforms(currentTile.modelMatrix, viewMtx, projMtx);

        _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.materialColor, currentTile.color);

        CSCI441::drawSolidCube(1.0);
    }
    //// END DRAWING THE TILES ////

    //// BEGIN DRAWING THE HERO ////
    glm::mat4 modelMtx(1.0f);
    // we are going to cheat and use our look at point to place our hero so that it is always in view
    modelMtx = glm::translate(modelMtx, _pArcballCam->getLookAtPoint() );
    // draw our hero now
    _pHero->drawHero(modelMtx, viewMtx, projMtx );
    //// END DRAWING THE HERO ////
}

void A3Engine::_updateScene() {
    // Handle the hero's forward movement and checks for environment boundaries.
    if(_keys[GLFW_KEY_W]) {
        heroPosition.x += cos(_pHero->getBodyAngle());
        heroPosition.z += -sin(_pHero->getBodyAngle());
        _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
        _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
        _pArcballCam->recomputeOrientation();

        if(heroPosition.x > 550.0f) {
            heroPosition.x = 550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.x < -550.0f) {
            heroPosition.x = -550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.z > 550.0f) {
            heroPosition.z = 550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.z < -550.0f) {
            heroPosition.z = -550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
    }

    // Handle the hero's backward movement and checks for environment boundaries.
    if(_keys[GLFW_KEY_S]) {
        heroPosition.x -= cos(_pHero->getBodyAngle());
        heroPosition.z -= -sin(_pHero->getBodyAngle());
        _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
        _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
        _pArcballCam->recomputeOrientation();

        if(heroPosition.x > 550.0f) {
            heroPosition.x = 550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.x < -550.0f) {
            heroPosition.x = -550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.z > 550.0f) {
            heroPosition.z = 550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
        if(heroPosition.z < -550.0f) {
            heroPosition.z = -550.0f;
            _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
            _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
            _pArcballCam->recomputeOrientation();
        }
    }

    // Rotates the hero's heading left or right.
    if(_keys[GLFW_KEY_D]) {
        _pHero->turnRight();
    }

    if(_keys[GLFW_KEY_A]) {
        _pHero->turnLeft();
    }

    // Creates our idle movement of hovering up and down.
    _hoverAmount = _yOffset * std::sin(M_PI/180 * _timeVariable);
    heroPosition.y += _hoverAmount;
    _timeVariable += 1;
    _pArcballCam->setPosition(glm::vec3(heroPosition.x, heroPosition.y, heroPosition.z) * 0.1f );
    _pArcballCam->setLookAtPoint(_pArcballCam->getPosition());
    _pArcballCam->recomputeOrientation();
}

void A3Engine::run() {
    //  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
    //	until the user decides to close the window and quit the program.  Without a loop, the
    //	window will display once and then the program exits.
    while( !glfwWindowShouldClose(mpWindow) ) {	        // check if the window was instructed to be closed
        glDrawBuffer( GL_BACK );				        // work with our back frame buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

        // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
        // when using a Retina display the actual window can be larger than the requested window.  Therefore,
        // query what the actual size of the window we are rendering to is.
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize( mpWindow, &framebufferWidth, &framebufferHeight );

        // update the viewport - tell OpenGL we want to render to the whole window
        glViewport( 0, 0, framebufferWidth, framebufferHeight );

        // draw everything to the window
        _renderScene(_pArcballCam->getViewMatrix(), _pArcballCam->getProjectionMatrix());

        _updateScene();

        glfwSwapBuffers(mpWindow);                       // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen
    }
}

//*************************************************************************************
//
// Private Helper FUnctions

void A3Engine::_computeAndSendMatrixUniforms(glm::mat4 modelMtx, glm::mat4 viewMtx, glm::mat4 projMtx) const {
    // precompute the Model-View-Projection matrix on the CPU
    glm::mat4 mvpMtx = projMtx * viewMtx * modelMtx;
    // then send it to the shader on the GPU to apply to every vertex
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.mvpMatrix, mvpMtx);

    // TODO #7: compute and send the normal matrix
    glm::mat3 normalMtx = glm::mat3(glm::transpose(glm::inverse(modelMtx)));
    _lightingShaderProgram->setProgramUniform(_lightingShaderUniformLocations.normalMatrix, normalMtx);

}

//*************************************************************************************
//
// Callbacks

void lab05_engine_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods ) {
    auto engine = (A3Engine*) glfwGetWindowUserPointer(window);

    // pass the key and action through to the engine
    engine->handleKeyEvent(key, action);
}

void lab05_engine_cursor_callback(GLFWwindow *window, double x, double y ) {
    auto engine = (A3Engine*) glfwGetWindowUserPointer(window);

    // pass the cursor position through to the engine
    engine->handleCursorPositionEvent(glm::vec2(x, y));
}

void lab05_engine_mouse_button_callback(GLFWwindow *window, int button, int action, int mods ) {
    auto engine = (A3Engine*) glfwGetWindowUserPointer(window);

    // pass the mouse button and action through to the engine
    engine->handleMouseButtonEvent(button, action);
}