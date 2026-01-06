#include "CVideoDeviceModel_Hypatia4.h"
#include "CVideoDeviceController.h"

CVideoDeviceModel_Hypatia4::CVideoDeviceModel_Hypatia4(DEVSELINFO *pDeviceSelfInfo) : CVideoDeviceModel(pDeviceSelfInfo)
{
}

int CVideoDeviceModel_Hypatia4::TransformDepthDataType(int nDepthDataType, bool bRectifyData) {
    return nDepthDataType == APC_DEPTH_DATA_14_BITS ? m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo().videoModeZ14:
                                                      m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo().videoModeD11OrColorOnly;
}

int CVideoDeviceModel_Hypatia4::AdjustZDTableIndex(int &nIndex) {
    nIndex = 0;
    return APC_OK; // Currently using scale down
}

int CVideoDeviceModel_Hypatia4::PreparePointCloudInfo() {
    constexpr unsigned short kFileIndexHypatia4 = 0;
    GetRectifyLogData(0, kFileIndexHypatia4, &m_rectifyLogData);
    GetPointCloudInfo(&m_rectifyLogData, m_pointCloudInfo, GetColorImageData(), GetDepthImageData());
    return APC_OK;
}
