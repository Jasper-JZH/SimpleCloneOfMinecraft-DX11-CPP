#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;


GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::FirstPerson)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_skyEffect. InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// 释放D2D的相关资源
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// 为D2D创建DXGI表面渲染目标
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugStringW(L"\n警告：Direct2D与Direct3D互操作性功能受限，你将无法看到文本信息。现提供下述可选方法：\n"
			L"1. 对于Win7系统，需要更新至Win7 SP1，并安装KB2670838补丁以支持Direct2D显示。\n"
			L"2. 自行完成Direct3D 10.1与Direct2D的交互。详情参阅："
			L"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			L"3. 使用别的字体库，比如FreeType。\n\n");
	}
	else if (hr == S_OK)
	{
		// 创建固定颜色刷和文本格式
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"黑体", nullptr, DWRITE_FONT_WEIGHT_MEDIUM,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// 报告异常问题
		assert(m_pd2dRenderTarget);
	}

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.01f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	}
}

void GameApp::UpdateScene(float dt)
{
	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	//更改手持方块
	// 1:草方块 2:石头 3:木板 4:木头 5:树叶 
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1)&&m_Blocks.curBlockType!=0)
		m_Blocks.ChangeBlockType(0);
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_Blocks.curBlockType != 1)
		m_Blocks.ChangeBlockType(1);
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3) && m_Blocks.curBlockType != 2)
		m_Blocks.ChangeBlockType(2);
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D4) && m_Blocks.curBlockType != 3)
		m_Blocks.ChangeBlockType(3);
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D5) && m_Blocks.curBlockType != 4)
		m_Blocks.ChangeBlockType(4);

	//*************************
	//人物移动类
	
	//更新人物浮空状态
	m_Player.UpdataFloatState(&m_Blocks);

	//获取要用到的第一人称摄像机信息
	XMFLOAT3 C1Pos = cam1st->GetPosition();
	XMFLOAT3 C1RightDir = cam1st->GetRightAxis();
	XMFLOAT3 C1ForwardDir = cam1st->GetLookAxis();
	C1ForwardDir.y = 0;

	//人物浮空时下落
	if (m_Player.isFloating)
	{
		C1Pos.y -= dt * 6;
		m_Player.Speed = 4;//降低空中移速
	}
	else if (m_Player.Speed < 6)m_Player.Speed = 6;//恢复移速

	static bool isMove = false;//人物是否有移动

	//跳跃
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Space) && !m_Player.isFloating)
	{
		isMove = true;
		//检测头顶是否有方块阻挡
		if(!m_Player.isUpCollosion(&m_Blocks))
		Timer1 = 1.3f;//开始倒计时
	}
	if (Timer1 > 0.0f)
	{
		C1Pos.y += dt * 12;
		Timer1 -= dt * 5;
	}
	else Timer1 = 0.0f;//计时器归零

	//更新第一人称摄像机y方向坐标
	cam1st->SetPosition(C1Pos.x, C1Pos.y, C1Pos.z);
	//更新人物模型位置到第一人称摄像机位置
	UpdataModelPos();

	//更新敌人状态
	for (int i = 0; i < EnemyCount; ++i)
	{
		if (m_Enemys[i].Heart <= 0)continue;//敌人已经被销毁就跳过

		//浮空状态
		if (m_Enemys[i].UpdataFloatState(&m_Blocks))
		{
			m_Enemys[i].isFloating = true;
		}
		else m_Enemys[i].isFloating = false;
		//浮空时下落
		if (m_Enemys[i].isFloating == true)
		{
			XMFLOAT3 pos = m_Enemys[i].GetTransform().GetPosition();
			m_Enemys[i].SetPosition(XMFLOAT3(pos.x, pos.y - 6 * dt, pos.z));
		}

		//计算敌人与玩家的水平距离并判断是否在互动范围内
		if (m_Enemys[i].CalDistToPlayer(XMFLOAT3(C1Pos.x, C1Pos.y, C1Pos.z)) < m_Enemys[i].TrackDist)
		{
			//敌人跟踪玩家
			m_Enemys[i].Trackplayer(&m_Blocks, &m_Player, dt);
		}
	}
	
	
	// 方向移动
	if (keyState.IsKeyDown(Keyboard::W))
	{
		//检测前方是否有方块阻挡
		if (!m_Player.isCollosion(&m_Blocks, C1ForwardDir))
		{
			isMove = true;
			cam1st->Walk(dt * m_Player.Speed);
		}
	}
	if (keyState.IsKeyDown(Keyboard::S))
	{
		//检测后方是否有方块阻挡
		if (!m_Player.isCollosion(&m_Blocks, XMFLOAT3(-C1ForwardDir.x, C1ForwardDir.y, -C1ForwardDir.z)))
		{
			isMove = true;
			cam1st->Walk(dt * -m_Player.Speed);
		}
	}
	if (keyState.IsKeyDown(Keyboard::A))
	{
		//检测左边是否有方块阻挡
		if (!m_Player.isCollosion(&m_Blocks, XMFLOAT3(-C1RightDir.x, C1RightDir.y, -C1RightDir.z)))
		{
			isMove = true;
			cam1st->Strafe(dt * -m_Player.Speed);
		}
	}
	if (keyState.IsKeyDown(Keyboard::D))
	{
		//检测右边是否有方块阻挡
		if (!m_Player.isCollosion(&m_Blocks, C1RightDir))
		{
			isMove = true;
			cam1st->Strafe(dt * m_Player.Speed);
		}
		
	}
	//人物手，腿模型转动动画
	static float Angle = 0.0f;
	if (isMove)
	{
		if (Angle > 1.2f || Angle < -1.2f)//超过一定角度时更改转动方向
			m_Player.m_righthand.isForward = !m_Player.m_righthand.isForward;
		if (m_Player.m_righthand.isForward)
			Angle += dt * 5;
		else Angle += dt * -5;
		//优先右手放置/销毁方块动画
		if (!m_Player.isMousePressed)
			m_Player.m_righthand.Rotate(Angle);
		m_Player.m_lefthand.Rotate(-Angle);
		m_Player.m_rightleg.Rotate(-Angle/2);
		m_Player.m_leftleg.Rotate(Angle/2);
		isMove = false;
	}

	//防止人物掉入虚空
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(0.0f), XMVectorSet(18.9f,100.0f,18.9f,0.0f)));
	cam1st->SetPosition(adjustedPos);


	//鼠标按键检测
	if (m_MouseTracker.leftButton == m_MouseTracker.PRESSED || m_MouseTracker.rightButton == m_MouseTracker.PRESSED)
	{
		if (Timer2 < 0.09)
		m_Player.isMousePressed = true;

		//获取屏幕中心发射出的射线
		Ray CenterRay = Ray::ScreenToRay(*m_pCamera, (float)m_ClientWidth / 2.0, (float)m_ClientHeight / 2.0);
		XMFLOAT3 RayHitPos = { 0.0f,0.0f,0.0f };//记录射线碰撞到的方块坐标

		//按下鼠标左键
		if (m_MouseTracker.leftButton == m_MouseTracker.PRESSED)
		{
			//优先检测攻击
			bool Attack = false;
			for (int i = 0; i < EnemyCount; i++)
			{
				if (m_Enemys[i].Heart <= 0)continue;
				if (m_Enemys[i].DistToPlayer < m_Enemys[i].TrackDist)//是否在攻击范围内
				{
					//射线与敌人包围盒碰撞检测
					if (CenterRay.Hit(m_Enemys[i].AABB))
					{
						m_Enemys[i].Heart -= 2;//敌人生命值减少
						Attack = true;
					}
				}
			}
			if (!Attack)//不是在攻击，则进行销毁方块相关
			{
				if (m_Player.GetRayHitPos(CenterRay.origin, CenterRay.direction, &m_Blocks))//检测是否有方块实例
					m_Player.LeftPressEvent(&m_Blocks);
			}
		}
		//按下鼠标右键
		else
		{
			if (m_Player.GetRayHitPos(CenterRay.origin, CenterRay.direction, &m_Blocks))//检测是否有方块实例
			{
				//射线与包围盒检测
				if (CenterRay.Hit(m_Blocks.GetBlockAABB(m_Player.HitPos), &m_Player.RayDist, m_Player.Maxdist))
					m_Player.RightPressEvent(&m_Blocks);
			}
		}
	}

	//人物右手动画
	if (m_Player.isMousePressed)
	{
		static float angle = -1.5f;
		if (Timer2 < -0.09)
		Timer2 = 0.3f;//开始计时
		if (Timer2 > 0)
		{
			if (Timer2 > 0.15f)angle += dt * 6;
			else angle -= dt * 6;
			m_Player.m_righthand.Rotate(angle);
			Timer2 -= dt;
		}
		else
		{
			angle = -1.5f;
			Timer2 = -0.1f;
			m_Player.isMousePressed = false;
		}
	}

	// 在鼠标没进入窗口前仍为ABSOLUTE模式
	if (mouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		cam1st->Pitch(mouseState.y * dt * 0.5f);
		cam1st->RotateY(mouseState.x * dt * 0.5f);
	}

	//第三人称摄像机操作
	if (m_CameraMode == CameraMode::ThirdPerson)
	{
		cam3rd->SetTarget(m_Player.m_body.GetTransform().GetPosition());
		//绕物体旋转
		if (mouseState.positionMode == Mouse::MODE_RELATIVE)
		{
			cam3rd->RotateX(mouseState.y * dt * 0.5f);
			cam3rd->RotateY(mouseState.x * dt * 0.5f);
			cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
			m_Player.Maxdist = cam3rd->GetDistance() + 4;
		}
	}
	else m_Player.Maxdist = 5.0f;

	// ******************
	// 更新摄像机相关
	//
	//更新观察矩阵
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());

	// 重置滚轮值
	m_pMouse->ResetScrollWheelValue();

	//摄像机模式切换
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::C))
	{
		if (m_CameraMode == CameraMode::FirstPerson)
		{
			cam3rd->SetRotationY(m_Player.m_body.GetTransform().GetRotation().y);
			m_pCamera = cam3rd;
			m_CameraMode = CameraMode::ThirdPerson;
		}
		else
		{
			m_pCamera = cam1st;
			m_CameraMode = CameraMode::FirstPerson;
		}
	}

	// 退出程序，这里应向窗口发送销毁信息
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);
	//*******************
	//3D部分
	//
	
	//清屏
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	
	//绘制实例
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderBlockInstance);
	m_Blocks.DrawBlockInstance(m_pd3dImmediateContext.Get(), m_BasicEffect);
	
	//绘制物体
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	//仅在第三人称摄像机下绘制完整人物动画
	if (m_CameraMode == CameraMode::ThirdPerson)
		m_Player.DrawPlayer(m_pd3dImmediateContext.Get(), m_BasicEffect);
	else if(m_Player.isMousePressed)
	{
		m_Player.m_righthand.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}
	//绘制敌人
	for (int i = 0; i < EnemyCount; ++i)
	{
		if (m_Enemys[i].Heart > 0)
		m_Enemys[i].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	}
	
	//绘制天空盒
	m_skyEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
	m_pSky->Draw(m_pd3dImmediateContext.Get(), m_skyEffect, *m_pCamera);


	//*******************
	//2D部分
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();

		//*************************
		//左上角信息

		//方块信息
		std::wstring text =
			L"当前方块:";
		UINT curBlockType = m_Blocks.curBlockType;
		switch (curBlockType)
		{
		case 0:text += L" 草方块\n";
			break;
		case 1:text += L" 石头\n";
			break;
		case 2:text += L" 木板\n";
			break;
		case 3:text += L" 木头\n";
			break;
		case 4:text += L" 树叶\n";
			break;
		}
		//敌人血量信息
		text += L"\n敌人1生命值: ";
		text += std::to_wstring(m_Enemys[0].Heart);
		text += L"\n敌人2生命值: ";
		text += std::to_wstring(m_Enemys[1].Heart);
		text += L"\n敌人3生命值: ";
		text += std::to_wstring(m_Enemys[2].Heart);
	
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());

		//*************************
		//屏幕中心光标
		std::wstring Crosshair = L"十";
		m_pd2dRenderTarget->DrawTextW(Crosshair.c_str(), (UINT32)Crosshair.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{(float)m_ClientWidth / 2 - 10, (float)m_ClientHeight / 2 - 10, (float)m_ClientWidth / 2 + 10,(float)m_ClientHeight / 2 + 10, }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化天空盒
	m_pSky = std::make_unique<SkyRender>();
	HR(m_pSky->InitResource(m_pd3dDevice.Get(), m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{
		L"..\\Texture\\posX.png", L"..\\Texture\\negX.png",
			L"..\\Texture\\posY.png", L"..\\Texture\\negY.png",
			L"..\\Texture\\posZ.png", L"..\\Texture\\negZ.png", },
		5000.0f));

	// 设置天空盒纹理
	m_BasicEffect.SetTextureCube(m_pSky->GetTextureCube());
	

	// ******************
	// 初始化游戏对象
	//
	
	//初始化地形
	InitLand();
	//初始化人物
	InitPlayer();
	//初始化敌人
	InitEnemy();

	// ******************
	// 初始化摄像机
	//
	
	//初始化第一人称视角摄像机
	cam1st = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	cam1st->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.01f, 1000.0f);
	cam1st->LookTo(XMFLOAT3(10.0f, 10.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	//初始化第三人称视角摄像机
	cam3rd = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	cam3rd->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	cam3rd->SetTarget(m_Player.m_body.GetTransform().GetPosition());
	cam3rd->SetDistance(5.0f);
	cam3rd->SetDistanceMinMax(2.0f, 8.0f);
	cam3rd->SetFrustum(XM_PI / 3, AspectRatio(), 0.01f, 1000.0f);
cam3rd->SetRotationX(XM_PIDIV2);
	m_pCamera = cam1st;//默认第一人称摄像机
	//着色器初始观察，投影矩阵
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	
	// ******************
	// 初始化不会变化的值
	//

	// 方向光
	DirectionalLight dirLight[4];//四个方向光
	dirLight[0].ambient= XMFLOAT4(0.47f, 0.47f, 0.47f, 1.0f);
	dirLight[0].diffuse= XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	dirLight[0].specular= XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].direction= XMFLOAT3(0.0f, -0.707f, -0.707f);
	//其余三个方向光的属性一致，仅方向不同
	dirLight[1] = dirLight[0];
	dirLight[1].direction= XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	//着色器设置光源资源
	for (int i = 0; i < 4; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);


	// ******************
	// 设置调试对象名
	//
	m_Blocks.SetDebugObjectName("Blocks");
	m_pSky->SetDebugObjectName("Sky");

	return true;
}

//初始化地形
void GameApp::InitLand()
{
	
	//初始化方块统一模型
	Model model;
		//设置网格模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(1.0f, 1.0f, 1.0f));
		//初始化光线计算相关值
	model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.reflect = XMFLOAT4();

	//创建方块纹理（导入方块纹理集到方块纹理数组中)
	HR(CreateBlockTextureArrayFromFile(
		m_pd3dDevice.Get(),
		m_pd3dImmediateContext.Get(),
		L"..\\Texture\\blockTextures.png",
		nullptr,
		mBlockTexArray.GetAddressOf()))
		//设置方块模型所用纹理数组
	model.modelParts[0].texArray = mBlockTexArray;
	m_Blocks.SetModel(std::move(model));//设置方块统一的模型

	//初始化每一种方块的实例数据
	// 草方块 0   木头3
	// 石头 1     树叶4
	// 木板 2     基岩5
	//底层
	for (int i = 0; i < 20; ++i)
	{
		for (int j = 0; j < 20; ++j)
		{
			m_Blocks.SetPosition(XMFLOAT3(i, 0, j), 5);
			m_Blocks.SetPosition(XMFLOAT3(i, 1, j), 1);
			m_Blocks.SetPosition(XMFLOAT3(i, 2, j), 1);
			m_Blocks.SetPosition(XMFLOAT3(i, 3, j), 0);
		}
	}
	//小山坡
	for (int lay = 0; lay < 3; ++lay)
	{
		for (int i = 3 + lay; i < 9 - lay; ++i)
		for (int j = 3 + lay; j < 9 - lay; ++j)
			m_Blocks.SetPosition(XMFLOAT3(i, 4+lay, j), 0);
	}
	//树
	CreateTree(XMFLOAT3(6, 7, 6), 2);
	CreateTree(XMFLOAT3(15, 4, 7), 3);
	CreateTree(XMFLOAT3(13, 4, 13), 4);
	CreateTree(XMFLOAT3(17, 4, 17), 3);

}

//初始化玩家人物模型
void GameApp::InitPlayer()
{
	Model model;
	//设置网格模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(0.5f, 0.7f, 0.5f));
	//初始化光线计算相关值
	model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.reflect = XMFLOAT4();

	
	//************************
	//设置玩家模型
	XMFLOAT3 defPos = { 5.0f, 5.0f, 5.0f };//人物模型默认初始位置
		//设置身体模型
	m_Player.m_body.SetModel(std::move(model));
	m_Player.m_body.SetPosition(defPos);
		//设置左手模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(0.2f, 0.6f, 0.2f));
	m_Player.m_lefthand.SetModel(std::move(model));
	m_Player.m_lefthand.SetPosition(XMFLOAT3(defPos.x-0.4f,defPos.y - 0.35f,defPos.z));
		//设置右手模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(0.2f, 0.6f, 0.2f));
	m_Player.m_righthand.SetModel(std::move(model));
	m_Player.m_righthand.SetPosition(XMFLOAT3(defPos.x + 0.4f, defPos.y - 0.35f, defPos.z));
		//设置左腿模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(0.25f, 1.0f, 0.25f));
	m_Player.m_leftleg.SetModel(std::move(model));
	m_Player.m_leftleg.SetPosition(XMFLOAT3(defPos.x - 0.2f, defPos.y - 1.0f, defPos.z));
		//设置右腿模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateBox(0.25f, 1.0f, 0.25f));
	m_Player.m_rightleg.SetModel(std::move(model));
	m_Player.m_rightleg.SetPosition(XMFLOAT3(defPos.x + 0.2f, defPos.y - 1.0f, defPos.z));
	

}

//初始化敌人
void GameApp::InitEnemy()
{
	Model model;
	//设置网格模型
	model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateCylinder(0.5f, 2.0f));
	//初始化光线计算相关值
	model.modelParts[0].material.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	model.modelParts[0].material.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	model.modelParts[0].material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	model.modelParts[0].material.reflect = XMFLOAT4();
	
	srand((unsigned)time(nullptr));
	XMFLOAT3 pos = { 0.0f,0.0f, 0.0f };
	for (int i = 0; i < EnemyCount; ++i)
	{
		model.SetMesh(m_pd3dDevice.Get(), Geometry::CreateCylinder(0.3f, 1.4f));
		m_Enemys[i].SetModel(std::move(model));
		//随机位置
		pos.x = (int)(18.0 * rand() / RAND_MAX + 1.0);
		pos.z = (int)(18.0 * rand() / RAND_MAX + 1.0);
		pos.y = 15;
		m_Enemys[i].SetPosition(XMFLOAT3(pos.x, pos.y, pos.z));
	}
}

//更新模型位置到第一人称摄像机位置
void GameApp::UpdataModelPos()
{
	
	m_Player.m_body.UpdataPosToCam(cam1st.get(), 0.0f, -0.2f, 0.0f );//身体主干部分始终绑定第一人称摄像机
	m_Player.m_righthand.UpdataPos(&m_Player.m_body, 0.4f, -0.35f, 0.0f);
	m_Player.m_lefthand.UpdataPos(&m_Player.m_body, -0.4f, -0.35f, 0.0f);
	m_Player.m_rightleg.UpdataPos(&m_Player.m_body, 0.2f, -1.0f, 0.0f);
	m_Player.m_leftleg.UpdataPos(&m_Player.m_body, -0.2f, -1.0f, 0.0f);

	//更新模型动画
}

//生成一棵树
void GameApp::CreateTree(XMFLOAT3 pos, int Height)
{
	if (Height <= 0)return;
	for (int k = 0; k < Height; k++)
		m_Blocks.SetPosition(XMFLOAT3(pos.x, pos.y + k, pos.z), 3);
	int lay = 0;
	for (int k = Height; k < Height + 3; k++, lay++)
		for (int i = -2 + lay; i < 3 - lay; i++)
			for (int j = -2 + lay; j < 3 - lay; j++)
				m_Blocks.SetPosition(XMFLOAT3(pos.x + i, pos.y + k, pos.z + j), 4);
}