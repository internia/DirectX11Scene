//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shader.h"
#include "Light.h"
#include "modelclass.h"
#include "RenderTexture.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    //audio
    void OnNewAudioDevice() noexcept { m_retryAudio = true; }

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    //Input controls
    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;
    
    //Light
    Light										m_Light;
    //Light										m_Light2;
    //Shaders
    Shader									m_BasicShaderPair;

    //scene elements
    std::unique_ptr<DirectX::GeometricPrimitive> m_room;

    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_proj;
    DirectX::SimpleMath::Matrix m_world;

    float m_pitch;
    float m_yaw;

    //camera elements
    DirectX::SimpleMath::Vector3 m_cameraPos;

    DirectX::SimpleMath::Color m_roomColor;

    std::shared_ptr<IEffect> effect;
    //textures
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_roomTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planet1Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planet2Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planet3Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_planet4Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallTex2;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture2;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> grassTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> marbleTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> exteriorTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fence;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> treeTrunkTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> treeTopTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorTex2;

    //audio
    std::unique_ptr<DirectX::AudioEngine> m_audEngine;
    bool m_retryAudio;
    std::unique_ptr<DirectX::SoundEffect> m_ambient;
    std::unique_ptr<DirectX::SoundEffectInstance> m_chill;
    float chillVolume;
    float chillSlide;

    //shapes and models
    std::unique_ptr<DirectX::GeometricPrimitive> m_planet1;
    std::unique_ptr<DirectX::GeometricPrimitive> m_planet2;
    std::unique_ptr<DirectX::GeometricPrimitive> m_planet3;
    std::unique_ptr<DirectX::GeometricPrimitive> m_planet4;
    std::unique_ptr<DirectX::GeometricPrimitive> m_stand;

    
    ModelClass                                                              standTop;
    ModelClass                                                              frame;

    ModelClass                                                              room2Ceiling;

    ModelClass																floor;
    ModelClass																floorRoom2;


    ModelClass                                                              wall1;
    ModelClass                                                              room2Wall1;

    ModelClass                                                              wall2Top;
    ModelClass                                                              wall2Right;
    ModelClass                                                              wall2Left;
    ModelClass                                                              room2Wall2;
    ModelClass                                                              room2Wall2Right;
    ModelClass                                                              room2Wall2Left;

    ModelClass                                                              wall3Top;
    ModelClass                                                              wall3Right;
    ModelClass                                                              wall3Left;
    ModelClass                                                              room2Wall3;

    ModelClass                                                              wall4;
    ModelClass                                                              room2Wall4;

    ModelClass                                                              outsideWallTop;
    ModelClass                                                              outsideWallRight;
    ModelClass                                                              outsideWallLeft;
    ModelClass                                                              outsideWallExtension;

    ModelClass                                                              roof1;
    ModelClass                                                              roof2;


    ModelClass                                                              outsideGround;
    ModelClass                                                              cobble;
    ModelClass                                                              fenceLeft;
    ModelClass                                                              fenceLeft1;
    ModelClass                                                              fenceLeft2;
    ModelClass                                                              fenceRight;
    ModelClass                                                              fenceRight1;
    ModelClass                                                              fenceRight2;

    ModelClass                                                              treeTop;
    ModelClass                                                              treeTrunk;


    //tank, anim and bones
    DirectX::ModelBone::TransformArray m_drawBones;
    DirectX::ModelBone::TransformArray m_animBones;
    uint32_t m_leftBackWheelBone;
    uint32_t m_rightBackWheelBone;
    uint32_t m_leftFrontWheelBone;
    uint32_t m_rightFrontWheelBone;
    uint32_t m_leftSteerBone;
    uint32_t m_rightSteerBone;
    uint32_t m_turretBone;
    uint32_t m_cannonBone;
    uint32_t m_hatchBone;

    //effects
    std::unique_ptr<DirectX::BasicEffect> m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::IEffectFactory> m_fxFactory;
    std::unique_ptr<DirectX::Model> m_model;
};
