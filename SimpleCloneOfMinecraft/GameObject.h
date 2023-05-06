//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Model.h"
#include "Transform.h"

class GameObject
{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


	GameObject() = default;
	~GameObject() = default;

	GameObject(const GameObject&) = default;
	GameObject& operator=(const GameObject&) = default;

	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	// 获取物体变换
	Transform& GetTransform();
	// 获取物体变换
	const Transform& GetTransform() const;

	//
	// 获取包围盒
	//

	DirectX::BoundingBox GetLocalBoundingBox() const;
	DirectX::BoundingBox GetBoundingBox() const;
	DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;

	
	//
	// 设置实例缓冲区
	//

	// 获取缓冲区可容纳实例的数目
	size_t GetCapacity() const;
	// 重新设置实例缓冲区可容纳实例的数目
	void ResizeBuffer(ID3D11Device * device, size_t count);
	// 获取实例缓冲区

	//
	// 设置模型
	//

	void SetModel(Model&& model);
	void SetModel(const Model& model);

	//
	// 绘制
	//

	// 绘制对象
	void Draw(ID3D11DeviceContext * deviceContext, BasicEffect& effect);
	// 绘制实例
	void DrawInstanced(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const std::vector<Transform>& data);

	//
	// 调试 
	//
	
	// 设置调试对象名
	// 若模型被重新设置，调试对象名也需要被重新设置
	void SetDebugObjectName(const std::string& name);

protected:
	Model m_Model = {};											    // 模型
	Transform m_Transform = {};										// 物体变换

	ComPtr<ID3D11Buffer> m_pInstancedBuffer = nullptr;				// 实例缓冲区
	size_t m_Capacity = 0;										    // 缓冲区容量

};

class Block ;//Block类声明

//***************************
//玩家对象
//
class Player
{
public:

	//管理人物身体模型
	class BodyPart
	{
	public:
		BodyPart() = default;
		~BodyPart() = default;
		// 获取模型变换
		Transform& GetTransform();
		// 获取模型变换
		const Transform& GetTransform() const;
		// 绘制模型对象
		void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);
		// 设置模型
		void SetModel(Model&& model);
		void SetModel(const Model& model);
		//设置模型位置
		void SetPosition(DirectX::XMFLOAT3 pos);
		//更新模型位置到摄像机位置
		void UpdataPosToCam(const Camera* CurCamera, float X = 0.0f, float Y = 0.0f, float Z = 0.0f);
		//更新模型位置
		void UpdataPos(const BodyPart* TargetPart, float X = 0.0f, float Y = 0.0f, float Z = 0.0f);
		//模型转动
		void Rotate(float Angle);
		//获取AABB包围盒
		DirectX::BoundingBox GetModelAABB();

		bool isForward = true;//（手，腿）模型转动时的前后方向

	private :
		Model m_Model = {};
		Transform m_Transform = {};
		
	};

	BodyPart m_body;
	BodyPart m_lefthand;
	BodyPart m_righthand;
	BodyPart m_leftleg;
	BodyPart m_rightleg;

private:

	bool isSelectBlock = false; //记录玩家当前是否选中方块
	float RayAdd = 0.1f;//每次射线向前移动的距离
	DirectX::XMFLOAT3 PlayerRayDir = { 0.0f,0.0f,0.0f };//玩家准星射线方向
	DirectX::XMFLOAT3 PlayerRayOri = { 0.0f,0.0f,0.0f };//玩家准星射线起点
	Block* pB = nullptr;
	DirectX::BoundingBox PlayerAABB;

public:

	bool isMousePressed = false;//按鼠标时右手要有动画
	float Speed = 6.0f;
	bool isFloating = false;//人物是否浮空
	float RayDist = 0.0f;//射线长度
	float Maxdist = 5.0f;//射线距离上限
	DirectX::XMFLOAT3 HitPos = { 0.0f,0.0f,0.0f };//射线碰撞到的方块的坐标
	bool GetRayHitPos(DirectX::XMFLOAT3 rayOri, DirectX::XMFLOAT3 rayDir, Block* pb);

	void LeftPressEvent(Block* pb);
	void RightPressEvent(Block* pb);

	void DrawPlayer(ID3D11DeviceContext* deviceContext, BasicEffect& effect);//绘制人物
	//获取人物AABB盒
	void GetPlayerAABB(DirectX::BoundingBox& PlayerAABB);
	//更新人物浮空状态
	void UpdataFloatState(Block* pb);
	//检测人物周围是否有方块阻挡
	bool isCollosion(Block* pb, DirectX::XMFLOAT3 BlockDir);
	//检测头顶是否有方块阻挡
	bool isUpCollosion(Block* pb);
	
	

};


//***************************
//方块类
//
class Block :public GameObject
{
	friend class Player;

private:
	
	//纹理索引数组
	typedef struct BlockInstanceData //用于存放所有方块的实例数据
	{
		Transform m_Transform;
		UINT ID;
	}BlockInstanceData;

	//用于key值的pos结构体，存放x,y,z坐标
	struct KeyPos
	{
		DirectX::XMFLOAT3 pos;
		KeyPos(DirectX::XMFLOAT3 POS) { this->pos = POS; };
		bool operator < (const KeyPos& right)const;
	};

	//用于维护的实例数据集，key为一个坐标向量
	std::map<KeyPos, BlockInstanceData> BlockData;  
	
public:

	UINT curBlockType = 0;//当前方块类型
	//设置方块位置
	void SetPosition(DirectX::XMFLOAT3 pos, UINT blockID);
	//更新实例缓冲区并绘制方块实例
	void DrawBlockInstance(ID3D11DeviceContext* deviceContext, BasicEffect& effect);
	//查找方块实例
	bool FindBlockInstance(KeyPos key, DirectX::XMFLOAT3& InstancePos);
	bool FindBlockInstance(DirectX::XMFLOAT3 keypos);
	//放置方块
	void PlaceBlock(DirectX::XMFLOAT3 newBlockPos);
	//销毁方块
	void RemoveBlock(DirectX::XMFLOAT3 BlockPos);
	//获取方块AABB盒
	DirectX::BoundingBox GetBlockAABB(DirectX::XMFLOAT3 SeletedBlockPos);
	//改变当前方块类型
	void ChangeBlockType(UINT newType);

};


//***************************
//敌人类
//
class Enemy
{

public:
	float TrackDist = 6.0f;//探测距离
	int Heart = 10;//生命值
	float DistToPlayer = 0.0f;//与玩家的距离
	DirectX::BoundingBox AABB;
public:
	Enemy() = default;
	~Enemy() = default;
	// 获取模型变换
	Transform& GetTransform();
	// 获取模型变换
	const Transform& GetTransform() const;
	// 设置模型
	void SetModel(Model && model);
	void SetModel(const Model & model);
	//设置模型位置
	void SetPosition(DirectX::XMFLOAT3 pos);
	//绘制敌人
	void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);
	//更新生命值
	void UpdataHeart(int dif);
	//浮空状态
	bool isFloating = true;
	//获取敌人AABB盒
	DirectX::BoundingBox GetEnemyAABB();

	//更新敌人浮空状态
	bool UpdataFloatState(Block* pb);
	//计算敌人与玩家的水平距离
	float CalDistToPlayer(DirectX::XMFLOAT3 PlayerPos);
	//跟踪玩家
	void Trackplayer(Block* pb, Player* pp, float dt);
	//是否受到到玩家攻击
	bool isAttacked();

private:
	DirectX::XMFLOAT3 PlayerDir = { 0.0f,0.0f, 0.0f};//玩家相对敌人的方向
	bool Stop = false;//是否被玩家或方块阻挡
	Model m_Model = {};
	Transform m_Transform = {};
};
#endif
