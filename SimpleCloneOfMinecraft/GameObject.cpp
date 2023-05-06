//#include "GameObject.h"
#include"GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include<utility>
#include<map>

using namespace DirectX;

struct InstancedData
{
	XMMATRIX world;
	XMMATRIX worldInvTranspose;
};

Transform& GameObject::GetTransform()
{
	return m_Transform;
}

const Transform& GameObject::GetTransform() const
{
	return m_Transform;
}

BoundingBox GameObject::GetBoundingBox() const
{
	BoundingBox box;
	m_Model.boundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

BoundingOrientedBox GameObject::GetBoundingOrientedBox() const
{
	BoundingOrientedBox box;
	BoundingOrientedBox::CreateFromBoundingBox(box, m_Model.boundingBox);
	box.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}


BoundingBox GameObject::GetLocalBoundingBox() const
{
	return m_Model.boundingBox;
}

size_t GameObject::GetCapacity() const
{
	return m_Capacity;
}

void GameObject::ResizeBuffer(ID3D11Device * device, size_t count)
{
	// 设置实例缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = (UINT)count * sizeof(InstancedData);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 创建实例缓冲区
	HR(device->CreateBuffer(&vbd, nullptr, m_pInstancedBuffer.ReleaseAndGetAddressOf()));

	// 重新调整m_Capacity
	m_Capacity = count;
}




void GameObject::SetModel(Model && model)
{
	std::swap(m_Model, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}

void GameObject::SetModel(const Model & model)
{
	m_Model = model;
}

void GameObject::Draw(ID3D11DeviceContext * deviceContext, BasicEffect & effect)
{
	UINT strides = m_Model.vertexStride;
	UINT offsets = 0;

	for (auto& part : m_Model.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
		effect.SetTextureDiffuse(part.texDiffuse.Get());
		effect.SetMaterial(part.material);
		effect.Apply(deviceContext);

		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

void GameObject::DrawInstanced(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const std::vector<Transform>& data)
{
	D3D11_MAPPED_SUBRESOURCE mappedData;
	UINT numInsts = (UINT)data.size();
	// 若传入的数据比实例缓冲区还大，需要重新分配
	if (numInsts > m_Capacity)
	{
		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(device.GetAddressOf());
		ResizeBuffer(device.Get(), numInsts);
	}

	HR(deviceContext->Map(m_pInstancedBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	InstancedData* iter = reinterpret_cast<InstancedData*>(mappedData.pData);
	for (auto& t : data)
	{
		XMMATRIX W = t.GetLocalToWorldMatrixXM();
		iter->world = XMMatrixTranspose(W);
		iter->worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
		iter++;
	}

	deviceContext->Unmap(m_pInstancedBuffer.Get(), 0);

	UINT strides[2] = { m_Model.vertexStride, sizeof(InstancedData) };
	UINT offsets[2] = { 0, 0 };
	ID3D11Buffer* buffers[2] = { nullptr, m_pInstancedBuffer.Get() };
	for (auto& part : m_Model.modelParts)
	{
		buffers[0] = part.vertexBuffer.Get();

		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新着色器中常量缓冲区的数据并应用
		//effect.SetTextureDiffuse(part.texDiffuse.Get());
		effect.SetTextureArray(part.texArray.Get());
		effect.SetMaterial(part.material);
		//effect.SetTexIndex(part.TexIndex_p_xyz, part.TexIndex_n_xyz);
		effect.Apply(deviceContext);

		deviceContext->DrawIndexedInstanced(part.indexCount, numInsts, 0, 0, 0);
	}
}


void GameObject::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)

	m_Model.SetDebugObjectName(name);
	if (m_pInstancedBuffer)
	{
		D3D11SetDebugObjectName(m_pInstancedBuffer.Get(), name + ".InstancedBuffer");
	}

#endif
}



//***********************************************************************************************
//class Block :public GameObject
//

//方块纹理数组切片
float TexIndexs[BlockTypeCount][6] =
{
	//草方块
	{0,1,2,3,4,5},
	//石头
	{6,7,8,9,10,11},
	//木板
	{12,13,14,15,16,17},
	//木头
	{18,19,20,21,22,23},
	//树叶
	{24,25,26,27,28,29},
	//基岩
	{30,31,32,33,34,35}

};

//绘制方块实例
void Block::DrawBlockInstance(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	D3D11_MAPPED_SUBRESOURCE mappedData;
	UINT numInsts = (UINT)BlockData.size();
	// 若传入的数据比实例缓冲区还大，需要重新分配
	if (numInsts > m_Capacity)
	{
		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(device.GetAddressOf());
		ResizeBuffer(device.Get(), numInsts);
	}

	//更新常量缓冲区（只用设置一次的值）
	effect.SetMaterial(m_Model.modelParts[0].material);
	effect.SetTextureArray(m_Model.modelParts[0].texArray.Get());
	effect.Apply(deviceContext);

	
	DirectX::XMFLOAT4 index_p_xyz, index_n_xyz;//临时索引数组变量
	index_p_xyz = index_n_xyz = { 0.0f ,0.0f ,0.0f ,0.0f };
	UINT EachIDInstanceCount = 0;
	//记录每种方块的实例数量

	
	for (UINT id = 0; id < BlockTypeCount; ++id)//循环绘制每一种方块
	{
		
		EachIDInstanceCount = 0;
		//根据ID确定纹理索引
		index_p_xyz.x = TexIndexs[id][0];
		index_p_xyz.y = TexIndexs[id][2];
		index_p_xyz.z = TexIndexs[id][4];
		index_n_xyz.x = TexIndexs[id][1];
		index_n_xyz.y = TexIndexs[id][3];
		index_n_xyz.z = TexIndexs[id][5];

		//将实例缓冲区映射到mappedData
		HR(deviceContext->Map(m_pInstancedBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		InstancedData* iter = reinterpret_cast<InstancedData*>(mappedData.pData);
		for (auto& t : BlockData)
		{
			if (t.second.ID != id)continue;
			EachIDInstanceCount++;
			XMMATRIX W = t.second.m_Transform.GetLocalToWorldMatrixXM();
			iter->world= XMMatrixTranspose(W);
			iter->worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));
			iter++;
		}
		deviceContext->Unmap(m_pInstancedBuffer.Get(), 0);
		UINT strides[2] = { m_Model.vertexStride, sizeof(InstancedData) };
		UINT offsets[2] = { 0, 0 };
		ID3D11Buffer* buffers[2] = { nullptr, m_pInstancedBuffer.Get() };

		for (auto& part : m_Model.modelParts)
		{
			buffers[0] = part.vertexBuffer.Get();
			// 设置顶点/索引缓冲区
			deviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
			deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

			//更新纹理索引到常量缓冲区
			effect.SetTexIndex(index_p_xyz, index_n_xyz);
			effect.Apply(deviceContext);
			
			//绘制当前Id方块的所有实例
			deviceContext->DrawIndexedInstanced(part.indexCount, EachIDInstanceCount, 0, 0, 0);
		}
	}
	
}

//设置方块位置
void Block::SetPosition(DirectX::XMFLOAT3 pos,UINT blockID)
{
	BlockInstanceData temp;
	temp.ID = blockID;
	temp.m_Transform.SetPosition(pos);
	KeyPos key(pos);
	BlockData.insert(std::pair<struct KeyPos, BlockInstanceData>(key, temp));
	return;
}

//Key值重载 < 运算符
bool Block::KeyPos::operator < (const KeyPos& right)const
{
	if (this->pos.x < right.pos.x)return true;
	else if (this->pos.x == right.pos.x)
	{
		if (this->pos.y < right.pos.y)return true;
		else if (this->pos.y == right.pos.y)
		{
			if (this->pos.z < right.pos.z)return true;
			else return false;
		}
		else return false;
	}
	else return false;
}

//查找方块实例
bool Block::FindBlockInstance(KeyPos key, DirectX::XMFLOAT3& InstancePos)
{
	std::map<KeyPos, BlockInstanceData>::iterator iter;
	iter = BlockData.find(key);
	if (iter != BlockData.end())//找到了
	{
		InstancePos = iter->second.m_Transform.GetPosition();
		return true;
	}
	else return false;
}
bool Block::FindBlockInstance(DirectX::XMFLOAT3 keypos)
{
	std::map<KeyPos, BlockInstanceData>::iterator iter;
	KeyPos key(keypos);
	iter = BlockData.find(key);
	if (iter != BlockData.end())//找到了
	{
		return true;
	}
	else return false;
}

//获取方块AABB碰撞盒
BoundingBox Block::GetBlockAABB(XMFLOAT3 SeletedBlockPos)
{
	KeyPos key(SeletedBlockPos);
	std::map<KeyPos, BlockInstanceData>::iterator iter;
	iter = BlockData.find(key);
	if (iter != BlockData.end())
	{
		BoundingBox box;
		m_Model.boundingBox.Transform(box, iter->second.m_Transform.GetLocalToWorldMatrixXM());
		return box;
	}
}

//删除方块
void Block::RemoveBlock(XMFLOAT3 BlockPos)
{
	KeyPos key(BlockPos);
	std::map<KeyPos, BlockInstanceData>::iterator iter;
	iter = BlockData.find(key); 
	if (iter != BlockData.end())
	{
		if (iter->second.ID == 5)return;//基岩不能破坏
		BlockData.erase(iter);//删除掉实例数据
	}
}

//放置方块
void Block::PlaceBlock(DirectX::XMFLOAT3 newBlockPos)
{
	return SetPosition(newBlockPos, curBlockType);
}

//改变手持方块
void Block::ChangeBlockType(UINT newType)
{
	curBlockType = newType;
	return;
}


//***********************************************************************************************
//class Player
//

//判断射线是否检测到方块，若有，更新HitPos
bool Player::GetRayHitPos(XMFLOAT3 rayOri, XMFLOAT3 rayDir,Block* pb)
{
	PlayerRayDir = rayDir;
	PlayerRayOri = rayOri;
	if (pb == nullptr)return false;
	isSelectBlock = false;
	XMFLOAT3 curRayPos = rayOri;//射线末端当前实际坐标
	XMFLOAT3 curRayBlockPos = { 0.0f,0.0f,0.0f };//射线末端当前取整后对应的方块位坐标
	XMFLOAT3 lastRayBlockPos = { 0.0f,0.0f,0.0f };//射线末端上次更新的取整后对应的方块位坐标
	int AddTimes =(int) Maxdist / RayAdd;//射线末端前进次数
	for (int i = 0; i < AddTimes; ++i)
	{
		//更新射线末端坐标
		curRayPos.x += RayAdd * rayDir.x;
		curRayPos.y += RayAdd * rayDir.y;
		curRayPos.z += RayAdd * rayDir.z;
		//射线末端所在方块（将坐标四舍五入为整数坐标以对应方块）
		curRayBlockPos.x = (float)round(curRayPos.x);
		curRayBlockPos.y = (float)round(curRayPos.y);
		curRayBlockPos.z = (float)round(curRayPos.z);
		//判断得到的方块坐标是否发生变化
		if (curRayBlockPos.x != lastRayBlockPos.x || curRayBlockPos.y != lastRayBlockPos.y || curRayBlockPos.z != lastRayBlockPos.z)
		{
			//判断当前射线末端坐标是否有实例方块
			if (pb->FindBlockInstance(curRayBlockPos, HitPos))
			{
				isSelectBlock = true;
				return true;
			}
			else lastRayBlockPos = curRayBlockPos;
		}
	}
	//达到最大距离还没有碰撞到方块实例
	return false;
}

//玩家左键事件
void Player::LeftPressEvent(Block* pb)
{
	if (!isSelectBlock)return;//没有检测到方块，无按键反应

	//删除方块
	pb->RemoveBlock(HitPos);
	isSelectBlock = false;//重置状态

}

//玩家右键事件
void Player::RightPressEvent(Block* pb)
{
	if (!isSelectBlock)return;//没有检测到方块，无按键反应

	//确实方块生成的位置
	XMFLOAT3 newBlockPos = { 0.0f,0.0f,0.0f };//新方块的坐标
	XMFLOAT3 realHitPos = { 0.0f,0.0f,0.0f };//射线与方块实际接触点坐标
	realHitPos.x = PlayerRayOri.x + PlayerRayDir.x * RayDist;
	realHitPos.y = PlayerRayOri.y + PlayerRayDir.y * RayDist;
	realHitPos.z = PlayerRayOri.z + PlayerRayDir.z * RayDist;

	XMFLOAT3 absDiff = { 0.0f,0.0f,0.0f };
	float value;
	absDiff.x = abs(realHitPos.x - HitPos.x);
	absDiff.y = abs(realHitPos.y - HitPos.y);
	absDiff.z = abs(realHitPos.z - HitPos.z);
	value = max(max(absDiff.x, absDiff.y), absDiff.z);//找到相对距离差最大的方向
	if (value == absDiff.x)
	{
		//利用正负判定相对位置
		if (realHitPos.x - HitPos.x < 0)
			newBlockPos.x = HitPos.x - 1;
		else newBlockPos.x = HitPos.x + 1;
		newBlockPos.y = HitPos.y;
		newBlockPos.z = HitPos.z;
	}
	else if (value == absDiff.y)
	{
		if (realHitPos.y - HitPos.y < 0)
			newBlockPos.y = HitPos.y - 1;
		else newBlockPos.y = HitPos.y + 1;
		newBlockPos.x = HitPos.x;
		newBlockPos.z = HitPos.z;
	}
	else
	{
		if (realHitPos.z - HitPos.z < 0)
			newBlockPos.z = HitPos.z - 1;
		else newBlockPos.z = HitPos.z + 1;
		newBlockPos.x = HitPos.x;
		newBlockPos.y = HitPos.y;
	}
	//生成新方块
	pb->PlaceBlock(newBlockPos);
	isSelectBlock = false;
}

//绘制人物
void Player::DrawPlayer(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	m_body.Draw(deviceContext, effect);
	m_lefthand.Draw(deviceContext, effect);
	m_righthand.Draw(deviceContext, effect);
	m_leftleg.Draw(deviceContext, effect);
	m_rightleg.Draw(deviceContext, effect);
}

//获取人物AABB盒
void Player::GetPlayerAABB(BoundingBox& PlayerAABB)
{
	BoundingBox::CreateMerged(PlayerAABB, m_lefthand.GetModelAABB(), m_rightleg.GetModelAABB());
}

//更新人物浮空状态
void Player::UpdataFloatState(Block* pb)
{
	//计算人物当前位置脚下方块
	XMFLOAT3 curPos = m_body.GetTransform().GetPosition();
	XMFLOAT3 DownBlockPos = { 0.0f,0.0f, 0.0f };
	DownBlockPos.x = (float)round(curPos.x);
	DownBlockPos.z = (float)round(curPos.z);
	DownBlockPos.y = (float)round(curPos.y - 1.5f);

	//查找是否存在方块实例
	if (!pb->FindBlockInstance(DownBlockPos))
		isFloating = true;
	else isFloating = false;
}

//检测人物与周围方块的碰撞
bool Player::isCollosion(Block* pb, XMFLOAT3 BlockDir)
{
	//人物位置
	XMFLOAT3 pPos = m_body.GetTransform().GetPosition();
	//计算出指定方向的方块位置
	XMFLOAT3 BlockPos = { 0.0f, 0.0f, 0.0f };
	BlockPos.x = (float)round(pPos.x + BlockDir.x);
	BlockPos.y = (float)round(pPos.y + BlockDir.y);
	BlockPos.z = (float)round(pPos.z + BlockDir.z);
	if (pb->FindBlockInstance(BlockPos) || pb->FindBlockInstance(XMFLOAT3(BlockPos.x, BlockPos.y - 1, BlockPos.z)))
	{
		return true;
	}
	else return false;
}

//检测人物与顶部方块的碰撞
bool Player::isUpCollosion(Block* pb)
{
	//检测人物当前位置头顶是否有方块
	XMFLOAT3 curPos = m_body.GetTransform().GetPosition();
	XMFLOAT3 UpBlockPos = { 0.0f,0.0f, 0.0f };
	UpBlockPos.x = (float)round(curPos.x);
	UpBlockPos.z = (float)round(curPos.z);
	UpBlockPos.y = (float)round(curPos.y + 1.0f);
	//查找是否存在方块实例
	if (pb->FindBlockInstance(UpBlockPos))
		return true;
	else return false;
}


//***********************************************************************************************
//class Player:: BodyPart 
//

//设置人物身体部分的模型
void Player::BodyPart::SetModel(Model&& model)
{
	std::swap(m_Model, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}
void  Player::BodyPart::SetModel(const Model& model)
{
	m_Model = model;
}

//获取人物身体部分的变化
Transform& Player::BodyPart::GetTransform()
{
	return m_Transform;
}
const Transform& Player::BodyPart::GetTransform() const
{
	return m_Transform;
}

//设置人物身体部分模型的位置
void Player::BodyPart::SetPosition(XMFLOAT3 pos)
{
	return m_Transform.SetPosition(pos);
}

//绘制人物身体部分模型
void Player::BodyPart::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	UINT strides = m_Model.vertexStride;
	UINT offsets = 0;

	for (auto& part : m_Model.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
		effect.SetTextureDiffuse(part.texDiffuse.Get());
		effect.SetMaterial(part.material);
		effect.Apply(deviceContext);

		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

//更新模型位置到摄像机位置
void Player::BodyPart::UpdataPosToCam(const Camera* CurCamera, float X, float Y , float Z)
{
	XMFLOAT3 CameraDir = CurCamera->GetLookAxis();//摄像机方向
	XMFLOAT3 CameraPos = CurCamera->GetPosition();//摄像机位置
	//计算摄像机的正方向轴到世界坐标系xoz面的投影
	XMFLOAT3 CameraDirProj = { CameraDir.x,0.0f,CameraDir.z };
	//设置模型部件位置与朝向
	m_Transform.SetPosition(XMFLOAT3(CameraPos.x + X, CameraPos.y + Y, CameraPos.z + Z));
	m_Transform.LookTo(CameraDirProj);
}

//更新模型位置
void Player::BodyPart::UpdataPos(const BodyPart* TargetPart, float X, float Y, float Z)
{
	Transform TargetTrans = TargetPart->GetTransform();
	XMVECTOR Pos = XMVector3Transform(XMVectorSet(X, Y, Z, 0.0f), TargetTrans.GetLocalToWorldMatrixXM());
	m_Transform.SetPosition(XMVectorGetX(Pos), XMVectorGetY(Pos), XMVectorGetZ(Pos));
	m_Transform.LookTo(TargetTrans.GetForwardAxis());
}

//模型转动
void Player::BodyPart::Rotate(float Angle)
{
	//XMMatrixRotationAxis()
	XMFLOAT3 Axis = m_Transform.GetRightAxis();
	XMFLOAT3 pos = m_Transform.GetPosition();
	m_Transform.RotateAround(XMFLOAT3(pos.x, pos.y + 0.5, pos.z), Axis, Angle);
}

//获取AABB包围盒
BoundingBox Player::BodyPart::GetModelAABB()
{
	BoundingBox box;
	m_Model.boundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}






//***********************************************************************************************
//class Enemy
//

//设置敌人模型
void Enemy::SetModel(Model&& model)
{
	std::swap(m_Model, model);
	model.modelParts.clear();
	model.boundingBox = BoundingBox();
}
void Enemy::SetModel(const Model& model)
{
	m_Model = model;
}

//获取敌人的变化
Transform& Enemy::GetTransform()
{
	return m_Transform;
}
const Transform& Enemy::GetTransform() const
{
	return m_Transform;
}

//设置敌人的位置
void Enemy::SetPosition(XMFLOAT3 pos)
{
	return m_Transform.SetPosition(pos);
}

//更新生命值
void Enemy::UpdataHeart(int dif)
{
	if (Heart > 0 && Heart <= 10)
	Heart += dif;
}

//绘制敌人
void Enemy::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
	UINT strides = m_Model.vertexStride;
	UINT offsets = 0;

	for (auto& part : m_Model.modelParts)
	{
		// 设置顶点/索引缓冲区
		deviceContext->IASetVertexBuffers(0, 1, part.vertexBuffer.GetAddressOf(), &strides, &offsets);
		deviceContext->IASetIndexBuffer(part.indexBuffer.Get(), part.indexFormat, 0);

		// 更新数据并应用
		effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
		effect.SetTextureDiffuse(part.texDiffuse.Get());
		effect.SetMaterial(part.material);
		effect.Apply(deviceContext);

		deviceContext->DrawIndexed(part.indexCount, 0, 0);
	}
}

//获取敌人AABB盒
BoundingBox Enemy::GetEnemyAABB()
{
	BoundingBox box;
	m_Model.boundingBox.Transform(box, m_Transform.GetLocalToWorldMatrixXM());
	return box;
}

//更新敌人浮空状态
bool Enemy::UpdataFloatState(Block* pb)
{
	//计算敌人当前位置脚下方块
	XMFLOAT3 curPos = this->m_Transform.GetPosition();
	XMFLOAT3 DownBlockPos = { 0.0f, 0.0f, 0.0f };
	DownBlockPos.x = (float)round(curPos.x);
	DownBlockPos.z = (float)round(curPos.z);
	DownBlockPos.y = (float)round(curPos.y - 0.7f);
	//方块实例是否存在
	if (pb->FindBlockInstance(DownBlockPos))
	{
		//获取方块碰撞盒
		BoundingBox BlockBox = pb->GetBlockAABB(DownBlockPos);
		//获取敌人碰撞盒
		AABB = GetEnemyAABB();
		//碰撞检测
		if (AABB.Intersects(BlockBox))
			return false;
	}
	return true;
}

//计算敌人与玩家的水平距离
float Enemy::CalDistToPlayer(XMFLOAT3 PlayerPos)
{
	XMFLOAT3 curPos = m_Transform.GetPosition();
	DistToPlayer= sqrt(pow(PlayerPos.x - curPos.x, 2) + pow(PlayerPos.z - curPos.z, 2));
	return DistToPlayer;
}

//跟踪玩家
void Enemy::Trackplayer(Block* pb, Player* pp, float dt)
{
	//计算玩家相对于敌人的方向
	XMFLOAT3 playerPos = pp->m_body.GetTransform().GetPosition();
	XMFLOAT3 curPos = m_Transform.GetPosition();
	XMFLOAT3 dif = { playerPos.x - curPos.x ,playerPos.y - curPos.y ,playerPos.z - curPos.z };
	XMVECTOR XMdir = XMVector3Normalize(XMVectorSet(dif.x, dif.y, dif.z, 1));
	XMFLOAT3 dir = { XMVectorGetX(XMdir),0,XMVectorGetZ(XMdir) };
	m_Transform.LookTo(dir);//更新敌人朝向

	//获取敌人包围盒
	/*AABB = GetEnemyAABB();*/

	//判断是否被方块阻挡
	//计算前方方块位置
	XMFLOAT3 BlockPos =
	{
		(float)round(curPos.x + dir.x),
		(float)round(curPos.y),
		(float)round(curPos.z + dir.z),
	};

	//判断前方是否有方块实例存在
	if (pb->FindBlockInstance(BlockPos))
	{
		//获取方块包围盒
		BoundingBox BlockBox = pb->GetBlockAABB(BlockPos);
		//碰撞检测
		if (AABB.Intersects(BlockBox))
			Stop = true;
		else Stop = false;
	}
	else Stop = false;
	if (Stop == false)//判断是否被玩家阻挡
	{
		//获取玩家包围盒
		BoundingBox PlayerBox = pp->m_body.GetModelAABB();
		//碰撞检测
		if (AABB.Intersects(PlayerBox))
			Stop = true;
	}

	if (Stop == false)
	m_Transform.SetPosition(curPos.x +dir.x * dt, curPos.y, curPos.z + dir.z * dt);
}
