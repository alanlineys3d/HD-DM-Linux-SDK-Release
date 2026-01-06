#include "CVideoDeviceModel_Hypatia2.h"
#include "CVideoDeviceController.h"

CVideoDeviceModel_Hypatia2::CVideoDeviceModel_Hypatia2(DEVSELINFO *pDeviceSelfInfo) : CVideoDeviceModel(pDeviceSelfInfo)
{
}

int CVideoDeviceModel_Hypatia2::TransformDepthDataType(int nDepthDataType, bool bRectifyData) {
    return (nDepthDataType == APC_DEPTH_DATA_14_BITS || nDepthDataType == APC_DEPTH_DATA_SCALE_DOWN_14_BITS) ?
           m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo().videoModeZ14:
           m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo().videoModeD11OrColorOnly;
}

int CVideoDeviceModel_Hypatia2::AdjustZDTableIndex(int &nIndex) {
    nIndex = 0;
    return APC_OK; // Currently using scale down
}

int CVideoDeviceModel_Hypatia2::AdjustZDTableByPointCloudInfo(PointCloudInfo& info) {

    int disparity_len = info.disparity_len;
    float focalLength = info.focalLength;

    if (disparity_len > 0) {
        memset(m_zdTableInfo.ZDTable, 0, sizeof(m_zdTableInfo.ZDTable));
        m_zdTableInfo.nTableSize = APC_ZD_TABLE_FILE_SIZE_11_BITS;

        m_zdTableInfo.ZDTable[0] = 0;
        m_zdTableInfo.ZDTable[1] = 0;
        m_zdTableInfo.nZNear = USHRT_MAX;
        m_zdTableInfo.nZFar = 0;

        m_zdTableInfo.nTableSize = disparity_len * 2;
        std::vector<float> dToW(disparity_len);
        memcpy(&dToW[0], info.disparityToW, disparity_len * sizeof(float));

        for (int j = 0; j < disparity_len; j++) {
            ((WORD*) m_zdTableInfo.ZDTable)[j] = __bswap_16((WORD)(focalLength / dToW[j]));

            unsigned short nZValue = (((unsigned short)m_zdTableInfo.ZDTable[j * 2]) << 8) + m_zdTableInfo.ZDTable[j * 2 + 1];
            if (nZValue) {
                m_zdTableInfo.nZNear = std::min(m_zdTableInfo.nZNear, nZValue);
                m_zdTableInfo.nZFar = std::max(m_zdTableInfo.nZFar, nZValue);
            }
        }

        int nZNear, nZFar;
        m_pVideoDeviceController->GetPreviewOptions()->GetZRange(nZNear, nZFar);
        m_pVideoDeviceController->GetPreviewOptions()->SetZRange(m_zdTableInfo.nZNear, nZFar);
        m_pVideoDeviceController->AdjustZRange();
    }

    return APC_OK;
}
int CVideoDeviceModel_Hypatia2::PreparePointCloudInfo() {
    constexpr unsigned short kFileIndexHypatia2And4 = 0;
    GetRectifyLogData(0, kFileIndexHypatia2And4, &m_rectifyLogData);
    GetPointCloudInfo(&m_rectifyLogData, m_pointCloudInfo, GetColorImageData(), GetDepthImageData());
    AdjustZDTableByPointCloudInfo(m_pointCloudInfo);
    return APC_OK;
}
