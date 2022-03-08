//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

//scene bounds
namespace
{
    const XMVECTORF32 START_POSITION = { 15.5f, -1.5f, 15.f, 180.f };
    const XMVECTORF32 ROOM_BOUNDS = { 50.f, 10.f, 42.f, 0.f };
    constexpr float ROTATION_GAIN = 0.003f;
    constexpr float MOVEMENT_GAIN = 0.04f;
}

//constructor
Game::Game() noexcept(false) :
    m_retryAudio(false),
    m_pitch(0),
    m_yaw(0),
    m_cameraPos(START_POSITION),
    m_roomColor(Colors::White),
    m_leftBackWheelBone(ModelBone::c_Invalid),
    m_rightBackWheelBone(ModelBone::c_Invalid),
    m_leftFrontWheelBone(ModelBone::c_Invalid),
    m_rightFrontWheelBone(ModelBone::c_Invalid),
    m_leftSteerBone(ModelBone::c_Invalid),
    m_rightSteerBone(ModelBone::c_Invalid),
    m_turretBone(ModelBone::c_Invalid),
    m_cannonBone(ModelBone::c_Invalid),
    m_hatchBone(ModelBone::c_Invalid)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }

    m_chill.reset();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    //setup light
    m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
    m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
   // m_Light.setPosition(10.0f, -10.0f, -10.0f);
    m_Light.setPosition(2.0f, 1.0f, 1.0f);
    m_Light.setDirection(-1.0f, -1.0f, 1.0f);

    //m_Light2.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
    //m_Light2.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
    //// m_Light.setPosition(10.0f, -10.0f, -10.0f);
    //m_Light2.setPosition(10.0f, -10.0f, -10.0f);
    //m_Light2.setDirection(-1.0f, -1.0f, 1.0f);
   
   
    //init keyboard and mouse movements
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    //audio setup
    #ifndef audio


        AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
    #ifdef _DEBUG
        eflags |= AudioEngine_Debug;
    #endif
        m_audEngine = std::make_unique<AudioEngine>(eflags);

        m_ambient = std::make_unique<SoundEffect>(m_audEngine.get(),
            L"chill.wav");

        m_chill = m_ambient->CreateInstance();
        m_chill->Play(true);

        chillVolume = 1.f;
        chillSlide = -0.1f;
    #endif // !audio


}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());
    auto time = static_cast<float>(timer.GetTotalSeconds());

    //GAME LOGIC GOES HERE.

    //mouse input functionality
    #ifndef mouse inputs
    auto mouse = m_mouse->GetState();

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
            * ROTATION_GAIN;

        m_pitch -= delta.y;
        m_yaw -= delta.x;
    }

    m_mouse->SetMode(mouse.leftButton
        ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
    #endif // !mouse inputs


    //keyboard input functionality
    #ifndef keyboard inputs


        auto kb = m_keyboard->GetState();
        if (kb.Escape)
        {
            ExitGame();
        }
        if (kb.Home)
        {
            m_cameraPos = START_POSITION.v;
            m_pitch = m_yaw = 0;
        }

        Vector3 move = Vector3::Zero;

        if (kb.Up || kb.W)
            move.y += 1.f;

        if (kb.Down || kb.S)
            move.y -= 1.f;

        if (kb.Left || kb.A)
            move.x += 1.f;

        if (kb.Right || kb.D)
            move.x -= 1.f;

        if (kb.PageUp || kb.Space)
            move.z += 1.f;

        if (kb.PageDown || kb.X)
            move.z -= 1.f;
    #endif // !keyboard inputs

    //camera movemments
    #ifndef camera movement
            Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.f);

            move = Vector3::Transform(move, q);

            move *= MOVEMENT_GAIN;

            m_cameraPos += move;

            Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f))- Vector3(0.1f, 0.1f, 0.1f);

            m_cameraPos = Vector3::Min(m_cameraPos, halfBound);
            m_cameraPos = Vector3::Max(m_cameraPos, -halfBound);

            // limit pitch to straight up or straight down
            constexpr float limit = XM_PIDIV2 - 0.01f;
            m_pitch = __max(-limit, m_pitch);
            m_pitch = __min(+limit, m_pitch);

            // keep longitude in sane range by wrapping
            if (m_yaw > XM_PI)
            {
                m_yaw -= XM_2PI;
            }
            else if (m_yaw < -XM_PI)
            {
                m_yaw += XM_2PI;
            }

            float y = sinf(m_pitch);
            float r = cosf(m_pitch);
            float z = r * cosf(m_yaw) ;
            float x = r * sinf(m_yaw);

            XMVECTOR lookAt = m_cameraPos + Vector3(x, y, z)  ;

            m_view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);

            
           // XMMatrixLoo
    #endif // !camera movement

    //audio setups
    #ifndef audio
                if (m_retryAudio)
                {
                    m_retryAudio = false;
                    if (m_audEngine->Reset())
                    {
                        // TODO: restart any looped sounds here
                        if (m_chill)
                            m_chill->Play(true);
                    }
                }
                else if (!m_audEngine->Update())
                {
                    if (m_audEngine->IsCriticalError())
                    {
                        m_retryAudio = true;
                    }
                }

                chillVolume += elapsedTime * chillSlide;
                if (chillVolume < 0.f)
                {
                    chillVolume = 0.f;
                    chillSlide = -chillSlide;
                }
                else if (chillVolume > 1.f)
                {
                    chillVolume = 1.f;
                    chillSlide = -chillSlide;
                }
                m_chill->SetVolume(chillVolume);
    #endif // !audio

    //animation and bone setup
    #ifndef animation
                float wheelRotation = time * 5.f;
                float steerRotation = sinf(time * 0.75f) * 0.5f;
                float turretRotation = sinf(time * 0.333f) * 1.25f;
                float cannonRotation = sinf(time * 0.25f) * 0.333f - 0.333f;
                float hatchRotation = __min(0.f, __max(sinf(time * 2.f) * 2.f, -1.f));

                XMMATRIX mat = XMMatrixRotationX(wheelRotation);
                m_animBones[m_leftFrontWheelBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_leftFrontWheelBone]);
                m_animBones[m_rightFrontWheelBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_rightFrontWheelBone]);
                m_animBones[m_leftBackWheelBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_leftBackWheelBone]);
                m_animBones[m_rightBackWheelBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_rightBackWheelBone]);

                mat = XMMatrixRotationX(steerRotation);
                m_animBones[m_leftSteerBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_leftSteerBone]);
                m_animBones[m_rightSteerBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_rightSteerBone]);

                mat = XMMatrixRotationY(turretRotation);
                m_animBones[m_turretBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_turretBone]);

                mat = XMMatrixRotationX(cannonRotation);
                m_animBones[m_cannonBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_cannonBone]);

                mat = XMMatrixRotationX(hatchRotation);
                m_animBones[m_hatchBone] = XMMatrixMultiply(mat,
                    m_model->boneMatrices[m_hatchBone]);



    #endif // !animation

          
    elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    //Begin rendering context
    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto time = static_cast<float>(m_timer.GetTotalSeconds());

  
    //Set Rendering states. 
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    context->RSSetState(m_states->CullClockwise());

    // Turn our shaders on,  set parameters
    m_BasicShaderPair.EnableShader(context);

    // RENDERING WORLD HERE
    
    //shapes and models
#ifndef shapes

    //room
    m_room->Draw(Matrix::Identity, m_view, m_proj, m_roomColor, m_roomTex.Get());

    //planet 1
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    SimpleMath::Matrix transform = Matrix::CreateTranslation(0.5f, -0.5f, -1.5f);
    SimpleMath::Matrix scale = Matrix::CreateScale(2.5f, 2.5f, 2.5f);
    SimpleMath::Matrix planet1Rotation = SimpleMath::Matrix::CreateRotationY(time);
    m_effect->SetWorld(m_world);
    m_world = m_world * planet1Rotation * transform * scale;
    // Turn our shaders on,  set parameters
   // m_BasicShaderPair.EnableShader(context);
    m_planet1->Draw(m_world, m_view, m_proj, Colors::BlanchedAlmond, m_planet1Tex.Get());

    //planet 2
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = Matrix::CreateTranslation(0.5f, 1.2f, 3.0f);
    scale = Matrix::CreateScale(1.5f, 1.5f, 1.5f);
    SimpleMath::Matrix planet2Rotation = SimpleMath::Matrix::CreateRotationY(-time);
    SimpleMath::Matrix transform2 = Matrix::CreateTranslation(0.5f, 0.5f, 1.0f);
    m_effect->SetWorld(m_world);
    m_world = m_world * transform2 * planet2Rotation * scale *transform;
    m_planet2->Draw(m_world, m_view, m_proj, Colors::BlanchedAlmond, m_planet2Tex.Get());
    
    // planet 3
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = Matrix::CreateTranslation(1.5f, -0.1f, 0.5f);
    transform2 = Matrix::CreateTranslation(-1.0f, 2.f, -1.0f);
    SimpleMath::Matrix planet3Rotation = SimpleMath::Matrix::CreateRotationZ(time);
    SimpleMath::Matrix spin = SimpleMath::Matrix::CreateRotationY(-time);
    scale = Matrix::CreateScale(1.0f, 1.0f, 1.0f);
    m_effect->SetWorld(m_world);
    m_world = m_world * transform2 * scale * planet3Rotation * spin * transform;
    m_planet3->Draw(m_world, m_view, m_proj, Colors::BlanchedAlmond, m_planet3Tex.Get());

    // planet 4
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = Matrix::CreateTranslation(2.5f, 0.5f, -1.0f);
    scale = Matrix::CreateScale(1.0f, 1.0f, 1.0f);
    m_effect->SetWorld(m_world);
    m_world = m_world * transform * scale;
    m_planet4->Draw(m_world, m_view, m_proj, Colors::BlanchedAlmond, m_planet4Tex.Get());

    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = Matrix::CreateTranslation(5.0f, -3.9f, -0.05f);
    scale = Matrix::CreateScale(3.0f, 1.0f, 3.0f);
    m_effect->SetWorld(m_world);
    m_world = m_world * transform * scale;
    m_stand->Draw(m_world, m_view, m_proj, Colors::BlanchedAlmond, marbleTex.Get());
#endif // !shapes


#ifndef drawing indoor room1

    //DONE
    //prepare transform for floor object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.0f, -4.9f, 0.0f);
    m_world = m_world * transform;
    //setup and draw floor
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, woodTex.Get());
    floor.Render(context);

    //DONE
   //prepare transform for floor object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.0f, 4.8f, 0.0f);
    m_world = m_world * transform;
    //setup and draw floor
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, woodTex.Get());
    roof1.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.0f, 0.0f, -7.5f);
    m_world = m_world * transform;
    //wall 1 - RIGHT WALL
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall1.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(5.0f, 2.45f, 0.0f);
    m_world = m_world * transform;
    //wall 2 - back wall 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall2Top.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(5.0f, -2.45f, 5.0f);
    m_world = m_world * transform;
    //wall 2 - back wall 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall2Right.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(5.0f, -2.55f, -5.0f);
    m_world = m_world * transform;
    //wall 2 - back wall 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall2Left.Render(context);


    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.0f, 2.5f, 7.5f);
    m_world = m_world * transform;
    //wall 3 - LEFT WALL top
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall3Top.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(22.0f, -2.5f, 7.5f);
    m_world = m_world * transform;
    //wall 3 - LEFT WALL side
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall3Right.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(9.0f, -2.5f, 7.5f);
    m_world = m_world * transform;
    //wall 3 - LEFT WALL side
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall3Left.Render(context);

      //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(24.8f, 0.0f, 0.0f);
    m_world = m_world * transform;
    //wall 2 - back wall 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex.Get());
    wall4.Render(context);

#endif // !drawing indoor room1

#ifndef render indoor room 2

    //DONE
    //prepare transform for floor object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-5.0f, -4.9f, 0.0f);
    m_world = m_world * transform;
    //setup and draw floor
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, floorTex2.Get());
    floorRoom2.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-5.0f, 0.0f, -7.5f);
    m_world = m_world * transform;
    //ROOM 2 wall 1 - RIGHT WALL
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall1.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(4.9f, 2.5f, 0.f);
    m_world = m_world * transform;
    //wall 3 - LEFT WALL top
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall2.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(4.9f, -2.5f, -5.f);
    m_world = m_world * transform;
    //wall 3 - LEFT WALL side
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall2Right.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(4.9f, -2.5f, 5.0f); 
    m_world = m_world * transform;
    //wall 3 - LEFT WALL side
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall2Left.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-5.0f, 0.0f, 7.5f);
    m_world = m_world * transform;
    //ROOM 2 wall 3 - RIGHT WALL
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall3.Render(context);

    //DONE
    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-15.0f, 0.0f, 0.0f);
    m_world = m_world * transform;
    //room 2 wall 4 - back wall 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Wall4.Render(context);

    //DONE
    //prepare transform for ceilintg object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(0.0f, 4.9f, 0.0f);
    m_world = m_world * transform;
    //setup and draw ceiling
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, wallTex2.Get());
    room2Ceiling.Render(context);

#endif //!render indoor room 2

#ifndef rendering outdoor area
    //prepare transform for ground  object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-7.0f, -4.9f, 15.0f);
    m_world = m_world * transform;
    //setup and draw floor
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, grassTex.Get());
    outsideGround.Render(context);

    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.0f, 2.5f, 7.6f);
    m_world = m_world * transform;
    //outside wall
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, exteriorTex.Get());
    outsideWallTop.Render(context);

    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(22.0f, -2.5f, 7.6f);
    m_world = m_world * transform;
    //outside wall
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, exteriorTex.Get());
    outsideWallRight.Render(context);

    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(9.0f, -2.5f, 7.6f);
    m_world = m_world * transform;
    //outside wall
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, exteriorTex.Get());
    outsideWallLeft.Render(context);

    //prepare transform for wall object. 
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(-5.0f,-0.0f, 7.6f);
    m_world = m_world * transform;
    //outside wall
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, exteriorTex.Get());
    outsideWallExtension.Render(context);



    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    transform = SimpleMath::Matrix::CreateTranslation(15.2f, -4.8f, 15.0f);
    m_world = m_world * transform;
    //path
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, cobbleTex.Get());
    cobble.Render(context);


#endif // !rendering outdoor area

#ifndef rendering fences
    //fence
  

    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    scale = Matrix::CreateScale(5.0f, 5.0f, 5.0f);
    SimpleMath::Matrix rotation = Matrix::CreateRotationY(300);
    transform = Matrix::CreateTranslation(11.f, -4.9f, 11.f);
    m_world = m_world * scale * rotation * transform;
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, fence.Get());
    fenceRight.Render(context);

    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    scale = Matrix::CreateScale(5.0f, 5.0f, 5.0f);
    rotation = Matrix::CreateRotationY(300);
    transform = Matrix::CreateTranslation(10.9f, -4.9f, 16.0f);
    m_world = m_world * scale * rotation * transform;
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, fence.Get());
    fenceRight1.Render(context);

    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    scale = Matrix::CreateScale(5.0f, 5.0f, 5.0f);
    rotation = Matrix::CreateRotationY(300);
    transform = Matrix::CreateTranslation(10.8f, -4.9f, 21.f);
    m_world = m_world * scale * rotation * transform;
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_proj, &m_Light, fence.Get());
    fenceRight2.Render(context);

#endif

#ifndef models
    m_world = SimpleMath::Matrix::Identity; //set world back to identity
    scale = Matrix::CreateScale(0.5f, 0.5f, 0.5f);
    transform = Matrix::CreateTranslation(30.5f, -5.7f, 0.5f);
    m_effect->SetWorld(m_world);
    m_world = m_world * transform * scale ;
    size_t nbones = m_model->bones.size();

    m_model->CopyAbsoluteBoneTransforms(nbones,m_animBones.get(), m_drawBones.get());

    m_model->Draw(context, *m_states, nbones, m_drawBones.get(), m_world, m_view, m_proj);
#endif // !models

#ifndef lighting
    auto ilights = dynamic_cast<IEffectLights*>(effect.get());
    if (ilights)
    {
        ilights->SetLightEnabled(0, true);  ilights->SetLightEnabled(1, true);

        static const XMVECTORF32 light{ 1.f, 1.f, 1.f, 0.f };
        static const XMVECTORF32 light2{ 10.f, 5.f, 5.f, 0.f };
        ilights->SetLightDirection(0, light);
        ilights->SetLightDirection(1, light2);

    }
#endif // !lighting

    
   
    context;

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // Game is becoming active window.
}

void Game::OnDeactivated()
{
    // Game is becoming background window.
}

void Game::OnSuspending()
{
    //Game is being power-suspended (or minimized).
    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // Game is being power-resumed (or returning from minimize).
    m_audEngine->Resume();
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // Change to desired default window size 
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).

    auto context = m_deviceResources->GetD3DDeviceContext();

    //setup shader
    m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");

    //effects
    #ifndef setup effects
        m_effect = std::make_unique<BasicEffect>(device);
        m_effect->SetTextureEnabled(true);
        m_effect->SetPerPixelLighting(true);
        m_effect->SetLightingEnabled(true);
        m_effect->SetLightEnabled(0, true);
        m_effect->SetLightDiffuseColor(0, Colors::White);
        m_effect->SetLightDirection(0, -Vector3::UnitZ);
    #endif // !setup effects

    //shapes and models
    #ifndef initialise and create all models and shapes
        m_room = GeometricPrimitive::CreateBox(context, XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]), false, true);
        m_planet1 = GeometricPrimitive::CreateSphere(context);
        m_planet2 = GeometricPrimitive::CreateSphere(context);
        m_planet3 = GeometricPrimitive::CreateSphere(context);
        m_planet4 = GeometricPrimitive::CreateSphere(context);
        m_stand = GeometricPrimitive::CreateCube(context, 2);
        floor.InitializeBox(device, 20.0f, 0.1f, 15.0f);	//box includes dimensions
        roof1.InitializeBox(device, 20.0f, 0.1f, 15.0f);	//box includes dimensions
        floorRoom2.InitializeBox(device, 20.0f, 0.1f, 15.0f);	//box includes dimensions

        room2Ceiling.InitializeBox(device, 30.0f, 0.1f, 15.0f);

        wall1.InitializeBox(device, 20.0f, 10.0f, 0.1f);	//box includes dimensions
        room2Wall1.InitializeBox(device, 20.0f, 10.0f, 0.1f);	//box includes dimensions

        wall2Top.InitializeBox(device, 0.1f, 5.0f, 15.0f);	//BACKWALL
        wall2Right.InitializeBox(device, 0.1f, 5.0f, 5.0f);	//BACKWALL
        wall2Left.InitializeBox(device, 0.1f, 5.0f, 5.0f);	//BACKWALL
        room2Wall2.InitializeBox(device, 0.1f, 5.0f, 15.0f);
        room2Wall2Right.InitializeBox(device, 0.1f, 5.0f, 5.0f);
        room2Wall2Left.InitializeBox(device, 0.1f, 5.0f, 5.0f);

        wall3Top.InitializeBox(device, 20.0f, 5.0f, 0.1f);	//rightwall
        wall3Right.InitializeBox(device, 8.0f, 5.0f, 0.1f);	//rightwall
        wall3Left.InitializeBox(device, 8.0f, 5.0f, 0.1f);	//rightwall
        room2Wall3.InitializeBox(device, 20.0f, 10.0f, 0.1f);


        wall4.InitializeBox(device, 0.1f, 10.0f, 15.0f);	//BACKWALL2
        room2Wall4.InitializeBox(device, 0.1f, 10.0f, 15.0f);

        outsideGround.InitializeBox(device, 65.0f, 0.1f, 15.0f); //grass
        outsideWallTop.InitializeBox(device, 20.0f, 5.0f, 0.1f);
        outsideWallRight.InitializeBox(device, 8.0f, 5.0f, 0.1f);	//rightwall
        outsideWallLeft.InitializeBox(device, 8.0f, 5.0f, 0.1f);	//rightwall
        outsideWallExtension.InitializeBox(device, 20.0f, 10.0f, 0.1f);
        cobble.InitializeBox(device, 5.0f, 0.1f, 15.0f); //path



        fenceLeft.InitializeModel(device, "fence.obj");
        fenceLeft1.InitializeModel(device, "fence.obj");
        fenceLeft2.InitializeModel(device, "fence.obj");
        fenceRight.InitializeModel(device, "fence.obj");
        fenceRight1.InitializeModel(device, "fence.obj");
        fenceRight2.InitializeModel(device, "fence.obj");

    #endif // !initialise and create all models and shapes

  
    //load in textures from file
    #ifndef textures
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"sky.dds", nullptr, m_roomTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"planet1.dds", nullptr, m_planet1Tex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"planet2.dds", nullptr, m_planet2Tex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"planet3.dds", nullptr, m_planet3Tex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"planet4.dds", nullptr, m_planet4Tex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"wallpaper.dds", nullptr, wallTex.ReleaseAndGetAddressOf()));    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"wood.dds", nullptr, woodTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"t.dds", nullptr, marbleTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"GRASS.dds", nullptr, grassTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"stone.dds", nullptr, exteriorTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"cobble.dds", nullptr, cobbleTex.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"white.dds", nullptr, fence.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"space.dds", nullptr, wallTex2.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"tile.dds", nullptr, floorTex2.ReleaseAndGetAddressOf()));
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"tree.dds", nullptr, treeTrunkTex.ReleaseAndGetAddressOf()));
    #endif // !textures

   
    #ifndef additional effects applications
            //setting effects on texture
            m_effect->SetTexture(m_planet1Tex.Get());

            m_states = std::make_unique<CommonStates>(device);

            m_fxFactory = std::make_unique<EffectFactory>(device);
    #endif // !additional effects applications


    //loading in tank mesh, setting up bones for animation
    #ifndef animation model and bones

                m_model = Model::CreateFromSDKMESH(device, L"tank.sdkmesh", *m_fxFactory, ModelLoader_CounterClockwise | ModelLoader_IncludeBones);
                const size_t nbones = m_model->bones.size();

                m_drawBones = ModelBone::MakeArray(nbones);
                m_animBones = ModelBone::MakeArray(nbones);

                m_model->CopyBoneTransformsTo(nbones, m_animBones.get());
                uint32_t index = 0;
                for (const auto& it : m_model->bones)
                {
                    if (_wcsicmp(it.name.c_str(), L"tank_geo") == 0)
                    {
                        // Need to recenter the model.
                      //  m_animBones[index] = XMMatrixIdentity();
                    }
                    else if (_wcsicmp(it.name.c_str(), L"l_back_wheel_geo") == 0) { m_leftBackWheelBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"r_back_wheel_geo") == 0) { m_rightBackWheelBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"l_front_wheel_geo") == 0) { m_leftFrontWheelBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"r_front_wheel_geo") == 0) { m_rightFrontWheelBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"l_steer_geo") == 0) { m_leftSteerBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"r_steer_geo") == 0) { m_rightSteerBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"turret_geo") == 0) { m_turretBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"canon_geo") == 0) { m_cannonBone = index; }
                    else if (_wcsicmp(it.name.c_str(), L"hatch_geo") == 0) { m_hatchBone = index; }
                    ++index;
                }

    #endif // !animation model and bones

    m_world = Matrix::Identity;
    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    //Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f), Vector3::Zero, Vector3::UnitY);
    m_proj = Matrix::CreatePerspectiveFieldOfView( XMConvertToRadians(70.f), float(size.right) / float(size.bottom), 0.01f, 100.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_room.reset();
    m_roomTex.Reset();
    m_planet1.reset();
    m_planet2.reset();
    m_planet3.reset();
    m_planet4.reset();
    m_planet1Tex.Reset();
    m_planet2Tex.Reset();
    m_planet3Tex.Reset();
    m_planet4Tex.Reset();
    wallTex.Reset();
    wallTex2.Reset();
     m_texture2.Reset();
     woodTex.Reset();
     grassTex.Reset();
     marbleTex.Reset();
    exteriorTex.Reset();
     cobbleTex.Reset();
     fence.Reset();
     treeTopTex.Reset();
    floorTex2.Reset();
    m_effect.reset();
    m_inputLayout.Reset();
    m_states.reset();
    m_fxFactory.reset();
    m_model.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
