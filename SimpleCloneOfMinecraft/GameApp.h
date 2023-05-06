#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "ObjReader.h"
#include "Collision.h"
#include "SkyRender.h"

#define EnemyCount 3

class GameApp : public D3DApp
{
public:
	// 摄像机模式
	enum class CameraMode { FirstPerson, ThirdPerson};
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void InitLand();//初始化地形
	void InitPlayer();//初始化玩家人物模型
	void InitEnemy();//初始化敌人
	void CreateTree(DirectX::XMFLOAT3 pos,int Height);//生成一棵树
	void UpdataModelPos();//实现人物模型动画
	
private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // 单色笔刷
	ComPtr<IDWriteFont> m_pFont;								// 字体
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// 文本格式

	Block m_Blocks;												//方块 
	Player m_Player;                                            //玩家
	Enemy m_Enemys[EnemyCount];								    //敌人
	
	ComPtr<ID3D11ShaderResourceView> mBlockTexArray;            //方块的纹理数组资源视图
	
	BasicEffect m_BasicEffect;								    // 对象渲染特效管理

	SkyEffect m_skyEffect;                                      //天空盒特效管理
	std::unique_ptr<SkyRender> m_pSky;                          //天空盒

	std::shared_ptr<Camera> m_pCamera;						    // 当前摄像机
	std::shared_ptr<FirstPersonCamera> cam1st;                  // 第一人称摄像机
	std::shared_ptr<ThirdPersonCamera> cam3rd;                  // 第三人称摄像机
	CameraMode m_CameraMode;									// 摄像机模式

	float Timer1 = 0.0f;                                        //计时器1
	float Timer2 = -0.1f;                                        //计时器2

};


#endif