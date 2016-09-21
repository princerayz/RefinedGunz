#include "stdafx.h"
#include <stdio.h>
#include <math.h>
#include <tchar.h>

#include "MXml.h"

#include "RealSpace2.h"
#include "RMesh.h"
#include "RMeshMgr.h"

#include "MDebug.h"

#include "RAnimationMgr.h"
#include "RVisualmeshMgr.h"

#include "MZFileSystem.h"
#include "fileinfo.h"

#include "RShaderMgr.h"

#ifndef _PUBLISH

#define __BP(i,n)	MBeginProfile(i,n);
#define __EP(i)		MEndProfile(i);

#else

#define __BP(i,n) ;
#define __EP(i) ;

#endif

_NAMESPACE_REALSPACE2_BEGIN

bool			RMesh::mHardwareAccellated = false;
unsigned int	RMesh::mNumMatrixConstant = 0;

bool			RMesh::m_bTextureRenderOnOff = true;
bool			RMesh::m_bVertexNormalOnOff = true;
bool			RMesh::m_bToolMesh	= false;
bool			RMesh::m_bSilhouette = false;
float			RMesh::m_fSilhouetteLength = 300.f;
int				RMesh::m_parts_mesh_loading_skip = 0;

bool			RRenderNodeMgr::m_bRenderBuffer = false;

RMesh::RMesh() 
{
	Init();
}

RMesh::~RMesh() 
{
	Destroy();
}

void RMesh::Init()
{
	m_id			= -1;

	m_data_num		= 0;

	m_max_frame[0]	= 0;
	m_max_frame[1]	= 0;

	m_frame[0]		= 0;
	m_frame[1]		= 0;

	m_pVisualMesh	= NULL;

	m_is_use_ani_set = false;
	m_mtrl_auto_load = true;
	m_is_map_object = false;

	m_bUnUsededCheck = false;

	m_pAniSet[0] = NULL;
	m_pAniSet[1] = NULL;

	m_parts_mgr = NULL;

	m_base_mtrl_mesh = NULL;

	m_data.reserve(MAX_MESH_NODE_TABLE);

	m_isScale = false;
	m_vScale = rvector(1.f,1.f,1.f);

	m_PickingType = pick_real_mesh;
	
	m_MeshWeaponMotionType	= eq_weapon_etc;

	m_LitVertexModel	= false;
	m_bEffectSort		= false;
	m_isPhysiqueMesh	= false;

	m_fVis = 1.0f;

	mbSkyBox = false;

	m_vBBMax = D3DXVECTOR3(-9999.f,-9999.f,-9999.f);
	m_vBBMin = D3DXVECTOR3( 9999.f, 9999.f, 9999.f);

	m_vBBMaxNodeMatrix = m_vBBMax;
	m_vBBMinNodeMatrix = m_vBBMin;

	m_vAddBipCenter = rvector(0,0,0);

	m_isMultiAniSet = false;
	m_isCharacterMesh = false;
	m_isNPCMesh = false;

	m_nSpRenderMode = 0;

	m_isMeshLoaded = false;

	m_pToolSelectNode = NULL;

}

void RMesh::Destroy()
{
	DelMeshList();

	if(m_parts_mgr) {
		delete m_parts_mgr;
		m_parts_mgr = NULL;
	}

	m_isMeshLoaded = false;
}

void RMesh::ReloadAnimation() 
{
	m_ani_mgr.ReloadAll();
}

float RMesh::GetMeshVis() 
{ 
	return m_fVis;	
}

void  RMesh::SetMeshVis(float vis) 
{ 
	m_fVis = vis; 
}

float RMesh::GetMeshNodeVis(RMeshNode* pNode) 
{
	if(pNode==NULL) return 1.f;

	return max(min(pNode->m_vis_alpha,m_fVis),0.f);
}

void RMesh::SetVisualMesh(RVisualMesh* vm) 
{ 
	m_pVisualMesh = vm; 
}

RVisualMesh* RMesh::GetVisualMesh() 
{ 
	return m_pVisualMesh; 
}

void RMesh::SetMtrlAutoLoad(bool b) 
{ 
	m_mtrl_auto_load = b;
}

bool RMesh::GetMtrlAutoLoad() 
{ 
	return m_mtrl_auto_load; 
}

void RMesh::SetMapObject(bool b) 
{ 
	m_is_map_object = b; 
}

bool RMesh::GetMapObject() 
{ 
	return m_is_map_object; 
}


const char* RMesh::GetFileName()
{
	return m_FileName.c_str();
}

void RMesh::SetFileName(const char* name)
{
	if(!name[0]) return;

	m_FileName = name;
}

void RMesh::SetBaseMtrlMesh(RMesh* pMesh) 
{
	m_base_mtrl_mesh = pMesh; 
}

void RMesh::SetScale(rvector& v) 
{
	m_vScale = v;
	m_isScale = true;
}

void RMesh::ClearScale() 
{
	m_vScale = rvector(1.f,1.f,1.f);
	m_isScale = false;
}

void RMesh::SetPickingType(RPickType type) 
{
	m_PickingType = type;
}

RPickType RMesh::GetPickingType() 
{
	return m_PickingType;
}

void RMesh::SetMeshWeaponMotionType(RWeaponMotionType t) 
{
	m_MeshWeaponMotionType = t;
}

RWeaponMotionType RMesh::GetMeshWeaponMotionType() 
{
	return m_MeshWeaponMotionType;
}

void RMesh::SetPhysiqueMeshMesh(bool b) 
{
	m_isPhysiqueMesh = b;
}

bool RMesh::GetPhysiqueMesh() 
{
	return m_isPhysiqueMesh;
}

bool RMesh::isVertexAnimation(RMeshNode* pNode) 
{
	RAnimation* pAniSet = GetNodeAniSet(pNode);

	if(pAniSet) 
		if( pAniSet->GetAnimationType() == RAniType_Vertex ) 
			return true;

	return false;
}

void RMesh::SetSpRenderMode(int mode) 
{
	m_nSpRenderMode = mode;
}

bool RMesh::CmpFileName(const char* name)
{
	if(!name[0]) return false;
	
	if(m_FileName == name )
		return true;
	return false;
}

const char* RMesh::GetName()
{
	return m_ModelName.c_str();
}

void RMesh::SetName(const char* name)
{
	if(!name[0]) return;

	m_ModelName = name;
}

bool RMesh::CmpName(const char* name)
{
	if(!name[0]) return false;

	if(m_ModelName == name )
		return true;

	return false;
}

void RMesh::GetMeshData(RMeshPartsType type,vector<RMeshNode*>& nodetable)
{
	RMeshNode*	pMesh = NULL;

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	while (it_obj !=  m_list.end()) {
		pMesh = (*it_obj);
		if(pMesh->m_PartsType==type) {
			nodetable.push_back(pMesh);
		}
		it_obj++;
	}
}

RMeshNode* RMesh::GetMeshData(RMeshPartsType type)
{
	RMeshNode*	pMesh = NULL;

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	while (it_obj !=  m_list.end()) {
		pMesh = (*it_obj);
		if(pMesh->m_PartsType==type)
			return pMesh;
		it_obj++;
	}
	return NULL;
}

RMeshNode* RMesh::GetMeshData(const char* name)
{
	RMeshNode*	pMesh = NULL;

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	while (it_obj !=  m_list.end()) {
		pMesh = (*it_obj);
		if(strcmp(pMesh->GetName(),name)==0)
			return pMesh;
		it_obj++;
	}
	return NULL;
}

RMeshNode* RMesh::GetPartsNode(const char* name)
{
	if(!m_parts_mgr)
		return NULL;

	return m_parts_mgr->GetPartsNode(name);
}

void RMesh::GetPartsNode(RMeshPartsType type,vector<RMeshNode*>& nodetable)
{
	if(!m_parts_mgr)
		return;

	m_parts_mgr->GetPartsNode(type,nodetable);
}

void RMesh::DelMeshList()
{
	if (m_list.empty())
		return;

	RMeshNodeHashList_Iter node =  m_list.begin();

	while (node !=  m_list.end()) {
		delete (*node);
		node =  m_list.Erase(node);
	}

	m_data_num = 0;
}

int RMesh::FindMeshId(RAnimationNode* pANode)
{
	if( pANode == NULL )
		return -1;

	int ret_id = -1;

	bool bReConnect = false;

	if(pANode->m_pConnectMesh != this)
		bReConnect = true;
	else if( pANode->m_node_id == -1 )
		bReConnect = true;

	if( bReConnect ) {

		ret_id = FindMeshIdSub( pANode );

		pANode->m_node_id = ret_id;
		pANode->m_pConnectMesh = this;

		return ret_id;
	}

	return pANode->m_node_id;
}

int RMesh::FindMeshParentId(RMeshNode* pMeshNode)
{
	if(pMeshNode==NULL)
		return -1;

	int ret_id = -1;

	if( pMeshNode->m_nParentNodeID == -1 ) {
		ret_id = _FindMeshId(pMeshNode->m_Parent);
		pMeshNode->m_nParentNodeID = ret_id;
		return ret_id;
	}
		
	return pMeshNode->m_nParentNodeID;
}

int RMesh::_FindMeshId(int e_name)
{
	if (m_list.empty())
		return -1;

	RMeshNode* pNode = m_list.Find(e_name);
	if (pNode != NULL)
		return pNode->m_id;

	return -1;
}

int RMesh::_FindMeshId(const char* name)
{
	if (m_list.empty())
		return -1;

	RMeshNode* pNode = m_list.Find(name);
	if (pNode != NULL)
		return pNode->m_id;

	return -1;
}

int RMesh::FindMeshId(RMeshNode* pNode)
{
	if(!pNode) return -1;

	if(pNode->m_NameID != -1)
		return _FindMeshId(pNode->m_NameID);

	return _FindMeshId(pNode->GetName());
}

int RMesh::FindMeshIdSub(RAnimationNode* pANode)
{
	if(!pANode) return -1;

	if(pANode->m_NameID != -1)
		return _FindMeshId(pANode->m_NameID);

	return _FindMeshId(pANode->GetName());
}

void __SetPosMat(D3DXMATRIX* m1,D3DXMATRIX* m2) {

	m1->_41 = m2->_41;
	m1->_42 = m2->_42;
	m1->_43 = m2->_43;
}

void __SetRotMat(D3DXMATRIX* m1,D3DXMATRIX* m2) {

	D3DXMATRIX m;

	m = *m1;
	*m1 = *m2;

	m1->_41 = m._41;
	m1->_42 = m._42;
	m1->_43 = m._43;
}

bool RMesh::ConnectMtrl()
{
	RMeshNode*	pMeshNode = NULL;

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	while (it_obj !=  m_list.end()) {
		pMeshNode = (*it_obj);

		if(pMeshNode) {
			if(pMeshNode->m_face_num)
				pMeshNode->ConnectMtrl();
		}

		it_obj++;
	}

	return NULL;
}

bool RMesh::ConnectAnimation(RAnimation* pAniSet)
{
	if(!pAniSet) 
		return false;

	RAnimationNode* pANode = NULL;

	int pid = -1;

	int node_cnt = pAniSet->GetAniNodeCount();

	for(int i=0;i< node_cnt;i++) {

		pANode = pAniSet->GetAniNode(i);

		pid = FindMeshId(pANode);
	}

	pAniSet->m_isConnected = true;

	return true;
}

bool RMesh::SetAnimation1Parts(RAnimation* pAniSet) {

	auto RMeshSetAnimation1Parts = MBeginProfile("RMesh::SetAnimation1Parts");

	bool bNodeHoldFrame = false;
	bool bNodeUpdate = false;

	if( m_pAniSet[1] )
		bNodeUpdate = true;
	
	if(m_pAniSet[0] == pAniSet) {
		if( pAniSet->m_isConnected ){
			if(m_pAniSet[0]->CheckName( pAniSet->GetName() ) ) {
				if(!bNodeUpdate) {
					return true;
				}
				else {
					bNodeHoldFrame = true;
				}
			}
		}
	}

	if(!pAniSet->m_isConnected) { 
		ConnectAnimation(pAniSet);
	}


	m_pAniSet[0] = pAniSet;
	m_pAniSet[1] = NULL;

	RAnimationNode* pANode = NULL;

	int pid = -1;

	m_is_use_ani_set = true;

	RMeshNode* pMeshNode = NULL;

	if(!bNodeHoldFrame)
		m_frame[0] = 0;

	int i=0;

	for(i=0;i<m_data_num;i++) {
		m_data[i]->m_pAnimationNode = NULL;
	}

	int node_cnt = pAniSet->GetAniNodeCount();

	for(i=0;i<node_cnt;i++) {

		pANode = pAniSet->GetAniNode(i);

		pid = FindMeshId(pANode);

		if(pid != -1) {

			RMeshNode* pM = m_data[pid];

			if(pM) {

				pM->m_pAnimationNode = pANode;
				memcpy(&pM->m_mat_base,&pANode->m_mat_base ,sizeof(D3DXMATRIX));
				memcpy(&pM->m_mat_local,&pM->m_mat_base,sizeof(D3DXMATRIX));
				RMatInv(pM->m_mat_inv,pM->m_mat_local);
			}
		}
	}

	if(pAniSet)
		m_max_frame[0] = pAniSet->GetMaxFrame();

	MEndProfile(RMeshSetAnimation1Parts);

	return true;
}

bool RMesh::SetAnimation2Parts(RAnimation* pAniSet,RAnimation* pAniSetUpper) {

	__BP(304, "RMesh::SetAnimation2Parts");

	int i=0;

	bool bC1 = true;
	bool bC2 = true;

	if(m_pAniSet[0] == pAniSet) {	
		if( pAniSet->m_isConnected ){
			if( m_pAniSet[0]->CheckName( pAniSet->GetName() ) ) {
				bC1 = false;
			}
		}
	}

	if(m_pAniSet[1] == pAniSetUpper) {	
		if( pAniSetUpper->m_isConnected ){
			if(m_pAniSet[1]->CheckName( pAniSetUpper->GetName() ) ) {
				bC2 = false;
			}
		}
	}

	if( !bC1 && !bC2 )
	{
		__EP(304);
		return true;
	}

	RAnimationNode* pANode = NULL;

	int pid = -1;

	m_is_use_ani_set = true;

	RMeshNode* pMeshNode = NULL;

	for(i=0;i<m_data_num;i++) {
		m_data[i]->m_pAnimationNode = NULL;
	}

	if( bC1 ) {
		if(!pAniSet->m_isConnected) {
			ConnectAnimation(pAniSet);
		}

		m_pAniSet[0] = pAniSet;
		m_frame[0] = 0;

		m_max_frame[0] = pAniSet->GetMaxFrame();
	}

	if( bC2 ) {
	
		if(!pAniSetUpper->m_isConnected) {
			ConnectAnimation(pAniSetUpper);
		}

		m_pAniSet[1] = pAniSetUpper;
		m_frame[1] = 0;

		m_max_frame[1] = pAniSetUpper->GetMaxFrame();
	}

	int node_cnt = pAniSet->GetAniNodeCount();

	for(i=0;i<node_cnt;i++) {

		pANode = pAniSet->GetAniNode(i);

		if(pANode){
		
			pid = FindMeshId(pANode);

			if(pid != -1) {

				RMeshNode* pM = m_data[pid];

				if(pM) {
					if(pM->m_CutPartsType == cut_parts_lower_body) {
						pM->m_pAnimationNode = pANode;
						memcpy(&pM->m_mat_base,&pANode->m_mat_base ,sizeof(D3DXMATRIX));
						memcpy(&pM->m_mat_local,&pM->m_mat_base,sizeof(D3DXMATRIX));
						RMatInv(pM->m_mat_inv,pM->m_mat_local);
					}
				}
			}
		}
	}

	node_cnt = pAniSetUpper->GetAniNodeCount();

	for(i=0;i<node_cnt;i++) {

		pANode = pAniSetUpper->GetAniNode(i);

		if(pANode) {

			pid = FindMeshId(pANode);

			if(pid != -1) {

				RMeshNode* pM = m_data[pid];

				if(pM) {
					if( pM->m_CutPartsType == cut_parts_upper_body ){
						pM->m_pAnimationNode = pANode;
						memcpy(&pM->m_mat_base,&pANode->m_mat_base ,sizeof(D3DXMATRIX));
						memcpy(&pM->m_mat_local,&pM->m_mat_base,sizeof(D3DXMATRIX));
						RMatInv(pM->m_mat_inv,pM->m_mat_local);

						if(pM->m_LookAtParts == lookat_parts_spine1) {
							rmatrix m,inv;
							RAnimationNode* pANodePa = pAniSetUpper->GetNode(pM->m_Parent);
							RMatInv(inv,pANodePa->m_mat_base);

							m = pANode->m_mat_base * inv;

							pM->m_spine_local_pos.x = m._41;
							pM->m_spine_local_pos.y = m._42;
							pM->m_spine_local_pos.z = m._43;
						}
					}
				}
			}
		}
	}

	RAnimationNode* pANode1 = pAniSet->GetNode("Bip01");
	RAnimationNode* pANode2 = pAniSetUpper->GetNode("Bip01");

	m_vAddBipCenter.x = 0.f;
	m_vAddBipCenter.y = 0.f;
	m_vAddBipCenter.z = 0.f;

	if( pANode1 && pANode2 ) {
		m_vAddBipCenter.x = pANode2->m_mat_base._41 - pANode1->m_mat_base._41;
		m_vAddBipCenter.y = pANode2->m_mat_base._42 - pANode1->m_mat_base._42;
		m_vAddBipCenter.z = pANode2->m_mat_base._43 - pANode1->m_mat_base._43;
	}

	__EP(304);

	return true;
}

bool RMesh::SetAnimation(RAnimation* pAniSet,RAnimation* pAniSetUpper)
{
	if(!pAniSet) return false;

	if(pAniSetUpper)
		return SetAnimation2Parts(pAniSet,pAniSetUpper);

	return SetAnimation1Parts(pAniSet);
}

bool RMesh::SetAnimation(char* name,char* ani_name_upper) {

	if(m_ani_mgr.m_list.empty())
		return false;

	RAnimation* pAniSet = NULL;
	RAnimation* pAniSetUpper = NULL;

	pAniSet = m_ani_mgr.GetAnimation(name,-1);

	if(ani_name_upper) {
		pAniSetUpper = m_ani_mgr.GetAnimation( ani_name_upper,-1 );
	}

	return SetAnimation(pAniSet,pAniSetUpper);
}

void RMesh::ClearAnimation()
{
	m_is_use_ani_set = false;
	m_pAniSet[0] = NULL;
	m_pAniSet[1] = NULL;
}

bool RMesh::Pick(int mx, int my, RPickInfo* pInfo, rmatrix* world_mat)
{
	int sw = RGetScreenWidth();
	int sh = RGetScreenHeight();

	auto matProj = RProjection;

	rvector v{
		(2.0f * mx / sw - 1) / matProj._11,
		-(2.0f * my / sh - 1) / matProj._22,
		1.0f };

	auto InverseView = Inverse(RView);

	rvector dir{
		v.x*InverseView._11 + v.y*InverseView._21 + v.z*InverseView._31,
		v.x*InverseView._12 + v.y*InverseView._22 + v.z*InverseView._32,
		v.x*InverseView._13 + v.y*InverseView._23 + v.z*InverseView._33 };

	auto pos = GetTransPos(InverseView);

	Normalize(dir);

	return CalcIntersectsTriangle(pos, dir, pInfo, world_mat);

}

bool RMesh::Pick(const rvector& pos, const rvector& dir,RPickInfo* pInfo,rmatrix* world_mat)
{
	return CalcIntersectsTriangle(pos, dir, pInfo, world_mat);
}

bool RMesh::Pick(const rvector* vInVec,RPickInfo* pInfo,rmatrix* world_mat)
{
	return Pick(vInVec[0],vInVec[1], pInfo, world_mat);
}

void RMesh::ClearMtrl() {
	m_mtrl_list_ex.DelAll();
}

void RMesh::CalcBoxNode(D3DXMATRIX* world_mat)
{
	m_vBBMax = D3DXVECTOR3(-9999.f,-9999.f,-9999.f);
	m_vBBMin = D3DXVECTOR3( 9999.f, 9999.f, 9999.f);

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	RMeshNode* pTMeshNode = NULL;

	while (it_obj !=  m_list.end()) {

		RMeshNode*	pMeshNode = (*it_obj);

		CalcNodeMatrixBBox(pMeshNode);//bbox

		pTMeshNode = UpdateNodeAniMatrix(pMeshNode);

		it_obj++;
	}

}

void RMesh::CalcBoxFast(D3DXMATRIX* world_mat)
{

}

void RMesh::CalcBox(D3DXMATRIX* world_mat)
{
	if( m_list.empty())	return;

	RMeshNode* pTMeshNode  = NULL;

	m_vBBMax = D3DXVECTOR3(-9999.f,-9999.f,-9999.f);
	m_vBBMin = D3DXVECTOR3( 9999.f, 9999.f, 9999.f);

	RMeshNodeHashList_Iter it_obj =  m_list.begin();

	while (it_obj !=  m_list.end()) {

		RMeshNode*	pMeshNode = (*it_obj);

		CalcNodeMatrixBBox(pMeshNode);//bbox

		pTMeshNode = UpdateNodeAniMatrix(pMeshNode);

		if(pTMeshNode->m_isDummyMesh) {//Bip,Bone,Dummy Skip
			it_obj++;
			continue;
		}

		if(pTMeshNode->m_face_num != 0) {// ���� �� * resultmat
			if(world_mat)
				pTMeshNode->CalcVertexBuffer(*world_mat,true);
		}

		it_obj++;
	}
}

void RMesh::CalcNodeMatrixBBox(RMeshNode* pNode)
{
	auto Trans = GetTransPos(pNode->m_mat_result);
	SubCalcBBox(
		&m_vBBMaxNodeMatrix,
		&m_vBBMinNodeMatrix,
		&Trans);
}

void RMesh::CalcBBox(D3DXVECTOR3* v) 
{
	SubCalcBBox( &m_vBBMax, &m_vBBMin, v);
}

void RMesh::SubCalcBBox(D3DXVECTOR3* max,D3DXVECTOR3* min,D3DXVECTOR3* v)
{
	if ( (max==NULL) || (min==NULL) || (v==NULL) ) return;

	min->x = min(min->x, v->x);
	min->y = min(min->y, v->y);
	min->z = min(min->z, v->z);

	max->x = max(max->x, v->x);
	max->y = max(max->y, v->y);
	max->z = max(max->z, v->z);
}

void RMesh::RenderBox(D3DXMATRIX* world_mat)
{
	draw_box(world_mat,m_vBBMax,m_vBBMin,0xffffffff);
}

rvector RMesh::GetOrgPosition()
{
	if (auto* pMNode = m_data[0])
		return GetTransPos(pMNode->m_mat_base);

	return{ 0, 0, 0 };
}

_NAMESPACE_REALSPACE2_END