#include "CVideoDeviceModel_80363.h"
#include "CEYSDDeviceManager.h"
#include "CVideoDeviceController.h"
#include "eSPDI.h"
#include <algorithm>
#include <CThreadWorkerManage.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <CImageDataModel.h>
#include "ColorPaletteConfig.h"
// Helper function to get thread ID if not already defined
#ifndef GETTID_DEFINED
#define GETTID_DEFINED
static pid_t get_tid() {
    return syscall(SYS_gettid);
}
#endif

CVideoDeviceModel_80363::CVideoDeviceModel_80363(DEVSELINFO *pDeviceSelfInfo) :
                        CVideoDeviceModel(pDeviceSelfInfo),
                        CVideoDeviceModel_Kolor(pDeviceSelfInfo) {
    m_bIsInterleaveStreamStarting = false;
}

int CVideoDeviceModel_80363::GetCurrentTemperature(float& fTemperature) {
    /*
     * OG02B10 Camera Sensor Temperature Reading (Datasheet Section 3.6)
     *
     * Registers:
     *   0x4417: TPM2 INTEGER - Integer part of temperature (Bit[7:0])
     *   0x4418: TPM2 DECIMAL - Decimal part of temperature (Bit[7:0])
     *
     * Combined value = (0x4417 << 8) | 0x4418
     *
     * Formula:
     *   If combined <= 0xC000: Temperature = combined / 256.0 (positive)
     *   If combined > 0xC000:  Temperature = -(combined - 0xC000) / 256.0 (negative)
     *
     * Valid range: -63.996°C (-3F.FF) to 192°C (C0.00)
     */
    constexpr unsigned short OG02B10_TEMP_INTEGER_REG = 0x4417;  // TPM2 INTEGER
    constexpr unsigned short OG02B10_TEMP_DECIMAL_REG = 0x4418;  // TPM2 DECIMAL
    constexpr unsigned short OG02B10_SENSOR_ID = 0xC0;           // OG02B10 I2C slave address

    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    int ret = APC_OK;
    unsigned short tempIntegerVal = 0;
    unsigned short tempDecimalVal = 0;

    // Read integer part (0x4417)
    ret = APC_GetSensorRegister(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1],
                                OG02B10_SENSOR_ID, OG02B10_TEMP_INTEGER_REG, &tempIntegerVal,
                                FG_Address_2Byte | FG_Value_1Byte, SENSOR_BOTH);
    if (ret != APC_OK) {
        qDebug("[80363] Failed to read OG02B10 temperature integer register 0x4417, ret=%d\n", ret);
        return ret;
    }

    // Read decimal part (0x4418)
    ret = APC_GetSensorRegister(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1],
                                OG02B10_SENSOR_ID, OG02B10_TEMP_DECIMAL_REG, &tempDecimalVal,
                                FG_Address_2Byte | FG_Value_1Byte, SENSOR_BOTH);
    if (ret != APC_OK) {
        qDebug("[80363] Failed to read OG02B10 temperature decimal register 0x4418, ret=%d\n", ret);
        return ret;
    }

    // Combine integer and decimal parts into 16-bit value
    // tempIntegerVal is high byte (bits 15-8), tempDecimalVal is low byte (bits 7-0)
    unsigned short tempCombined = ((tempIntegerVal & 0xFF) << 8) | (tempDecimalVal & 0xFF);

    // Convert according to OG02B10 datasheet formula
    if (tempCombined <= 0xC000) {
        // Positive temperature: T(°C) = tempCombined / 256
        fTemperature = static_cast<float>(tempCombined) / 256.0f;
    } else {
        // Negative temperature: T(°C) = -((tempCombined - 0xC000) / 256)
        fTemperature = -static_cast<float>(tempCombined - 0xC000) / 256.0f;
    }

    qDebug("[80363] OG02B10 Temperature: integer=0x%02x, decimal=0x%02x, combined=0x%04x, temp=%.2f°C\n",
           tempIntegerVal, tempDecimalVal, tempCombined, fTemperature);

    return APC_OK;
}

bool CVideoDeviceModel_80363::IsCurrentModeDepthOnly() {
    // First, ensure we have the controller and options objects
    if (!m_pVideoDeviceController || !m_pVideoDeviceController->GetPreviewOptions() ||
        !m_pVideoDeviceController->GetModeConfigOptions()) {
        return false; // Cannot determine without controller/options
    }

    // Check if this model instance represents the target device (eSP936/80363)
    // Using IsESP936Series() which should be implemented in this class
    if (!IsESP936Series()) {
        return false; // Not the correct device type
    }

    // Check if Mode Config UI is currently active
    bool bIsModeConfig = m_pVideoDeviceController->GetPreviewOptions()->IsModeConfig();

    if (bIsModeConfig) {
        // Get the details of the currently selected mode from ModeConfigOptions
        ModeConfig::MODE_CONFIG currentModeInfo = m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo();

        // Check if the mode description is exactly "D"
        return (currentModeInfo.csModeDesc == "D");
    } else {
        // If Mode Config is not active, assume the restriction doesn't apply
        // (User is selecting streams manually, Kolor stream should behave normally)
        return false;
    }
}

int CVideoDeviceModel_80363::StartStreamingTask(){
    int ret = 0;
    auto *previewOption = m_pVideoDeviceController->GetPreviewOptions();

    bool bKolorStream = previewOption->IsStreamEnable(STREAM_KOLOR);
    bool bTrackStream = previewOption->IsStreamEnable(STREAM_TRACK);

    if (bKolorStream) {
        ret |= CreateStreamTask(STREAM_KOLOR);
    }

    /**
     * eSP936 use only one endpoint in the interleaved mode, which is different than eSP876.
     */
    if (IsInterleaveMode()) {
        m_bIsInterleaveStreamStarting = true;
        // Store previous values to restore them later
        m_bPrevLowLightValue = m_cameraPropertyModel[0]->GetCameraProperty(CCameraPropertyModel::LOW_LIGHT_COMPENSATION).nValue;
        m_cameraPropertyModel[0]->SetCameraPropertyValue(CCameraPropertyModel::LOW_LIGHT_COMPENSATION, 0);

        // Instead of setting "support" to false (which hides the UI element),
        // set "valid" to false (which grays out the UI element while keeping it visible)

        // Gray out camera properties for interleave mode
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::LOW_LIGHT_COMPENSATION, false);
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::AUTO_WHITE_BLANCE, false);
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::WHITE_BLANCE_TEMPERATURE, false);
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::AUTO_EXPOSURE, false);
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::EXPOSURE_TIME, false);
        m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::LIGHT_SOURCE, false);

        return ret;
    }

    sleep(2);

    if (bTrackStream) {
        ret |= CreateStreamTask(STREAM_TRACK);
    }

    return ret;
}

void CVideoDeviceModel_80363::UpdateImageProcessor() {
    m_pVideoDeviceController->UpdateImageProcessor(m_imageData[STREAM_KOLOR].nWidth, m_imageData[STREAM_KOLOR].nHeight,
                                                   m_imageData[STREAM_TRACK].nWidth, m_imageData[STREAM_TRACK].nHeight,
                                                   APCImageType::DepthDataTypeToDepthImageType(
                                                           m_imageData[STREAM_TRACK].depthDataType));
}

bool CVideoDeviceModel_80363::IsRectifyData() {
    if (m_pVideoDeviceController && m_pVideoDeviceController->GetModeConfigOptions()) {
        return m_pVideoDeviceController->GetModeConfigOptions()->GetCurrentModeInfo().bRectifyMode;
    }
    return false;
}

std::vector<CVideoDeviceModel::STREAM_TYPE> CVideoDeviceModel_80363::GetDepthAccuracyStreamTypes()
{
    // 80363 (eSP936/ORANGE) uses STREAM_TRACK on the mono path as its depth window.
    // Include STREAM_TRACK for depth accuracy when the track stream is available.
    std::vector<STREAM_TYPE> types = CVideoDeviceModel::GetDepthAccuracyStreamTypes();
    if (IsStreamSupport(STREAM_TRACK)) {
        types.insert(types.begin(), STREAM_TRACK);
    }
    return types;
}

bool CVideoDeviceModel_80363::IsStreamSupport(STREAM_TYPE type)
{
    switch (type){
        case STREAM_COLOR:
        case STREAM_DEPTH:
        case STREAM_KOLOR:
        case STREAM_TRACK:
            return !GetStreamInfoList(type).empty();
        default:
            return false;
    }
}
bool CVideoDeviceModel_80363::IsStreamAvailable()
{
    bool bColorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_COLOR);
    bool bDepthStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_DEPTH);
    bool bKolorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    bool bTrackStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK);

    return bColorStream || bDepthStream || bKolorStream || bTrackStream;
}
int CVideoDeviceModel_80363::UpdateDepthDataType() {
    int ret;
    unsigned short originalDataType = m_depthDataType;
    RETRY_APC_API(ret, APC_GetDepthDataType(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                            m_deviceSelInfo[INDEX_COLOR_PATH1], &m_depthDataType));

    if (APC_OK != ret) {
        return ret;
    }

    return APC_OK;
}

APCImageType::Value CVideoDeviceModel_80363::GetDepthImageType() {
    return ESP936VideoModeToImageType(GetDepthDataType());
}

bool CVideoDeviceModel_80363::IsESP936Series() {
    return true;
}

bool CVideoDeviceModel_80363::ModuleSyncSupport()
{
    if (IsInterleaveMode()) return false;

    bool bKolorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    if (bKolorStream && m_imageData[STREAM_KOLOR].bMJPG) return false;

    return true;
}

bool CVideoDeviceModel_80363::GetModuleSyncMasterCapability()
{
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    bool bCanBeMaster = false;
    int ret = APC_GetModuleSyncCapability(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1], &bCanBeMaster);
    if (ret != APC_OK) return false;
    return bCanBeMaster;
}

int CVideoDeviceModel_80363::ModuleSync()
{
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    return APC_SetModuleSyncTimingCountEnable(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1], true);
}

int CVideoDeviceModel_80363::SetModuleSyncForceSync(bool bEnable)
{
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    return APC_SetModuleSyncForceSync(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1], bEnable);
}

int CVideoDeviceModel_80363::SetModuleSyncTimingCount(bool bEnable)
{
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    return APC_SetModuleSyncTimingCountEnable(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1], bEnable);
}

int CVideoDeviceModel_80363::SetDepthDataType(int nDepthDataType){
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    if (!pEYSDI || !m_deviceSelInfo[INDEX_COLOR_PATH1]) {
        return APC_Init_Fail;
    }

    int ret = APC_Init_Fail;
    RETRY_APC_API(ret, APC_SetDepthDataType(pEYSDI, m_deviceSelInfo[INDEX_COLOR_PATH1], nDepthDataType));

    if (ret != APC_OK) {
        return ret;
    }

    UpdateDepthDataType();

    return APC_OK;
}


int CVideoDeviceModel_80363::GetPointCloudInfo(eSPCtrl_RectLogData *pRectifyLogData, PointCloudInfo &pointCloudInfo,
                                              ImageData colorImageData, ImageData depthImageData)
{
    memset(&pointCloudInfo, 0, sizeof(PointCloudInfo));
    const float ratio_Mat = (float) depthImageData.nHeight / (float) pRectifyLogData->OutImgHeight;
    const float baseline  = 1.0f / pRectifyLogData->ReProjectMat[14];
    const float diff      = pRectifyLogData->ReProjectMat[15] * ratio_Mat;

    pointCloudInfo.centerX = -1.0f * pRectifyLogData->ReProjectMat[3] * ratio_Mat;
    pointCloudInfo.centerY = -1.0f * pRectifyLogData->ReProjectMat[7] * ratio_Mat;
    pointCloudInfo.focalLength = pRectifyLogData->ReProjectMat[11] * ratio_Mat;
    pointCloudInfo.fx1 = pRectifyLogData->NewCamMat1[0] * ratio_Mat;
    pointCloudInfo.fy1 = pRectifyLogData->NewCamMat1[5] * ratio_Mat;
    pointCloudInfo.fx2 = pRectifyLogData->NewCamMat2[0] * ratio_Mat;
    pointCloudInfo.fy2 = pRectifyLogData->NewCamMat2[5] * ratio_Mat;
    pointCloudInfo.cx1 = pRectifyLogData->NewCamMat1[2] * ratio_Mat;
    pointCloudInfo.cy1 = pRectifyLogData->NewCamMat1[6] * ratio_Mat;
    pointCloudInfo.cx2 = pRectifyLogData->NewCamMat2[2] * ratio_Mat;
    pointCloudInfo.cy2 = pRectifyLogData->NewCamMat2[6] * ratio_Mat;
    pointCloudInfo.Tx = -1 * baseline;
    pointCloudInfo.depthScaleRatio = ratio_Mat;

    switch (ESP936VideoModeToImageType(m_depthDataType)){
        case APCImageType::DEPTH_14BITS:{
            pointCloudInfo.disparity_len = 0;
            pointCloudInfo.wDepthType = APC_DEPTH_DATA_14_BITS;
            break;
        }
        case APCImageType::DEPTH_11BITS: {
            pointCloudInfo.disparity_len = 2048;
            for(int i = 0 ; i < pointCloudInfo.disparity_len ; ++i){
                pointCloudInfo.disparityToW[i] = ( i * ratio_Mat / 8.0f ) / baseline + diff;
            }
            pointCloudInfo.wDepthType = APC_DEPTH_DATA_11_BITS;
            break;
        }
        default:
            pointCloudInfo.disparity_len = 256;
            for(int i = 0 ; i < pointCloudInfo.disparity_len ; ++i){
                pointCloudInfo.disparityToW[i] = (i * ratio_Mat) / baseline + diff;
            }
            break;
    }

    return APC_OK;
}

int CVideoDeviceModel_80363::CloseDevice()
{
    bool bKolorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    bool bTrackStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK);

    int ret = APC_NoDevice;

    if (bTrackStream && APC_OK == APC_CloseDevice(CEYSDDeviceManager::GetInstance()->GetEYSD(), m_deviceSelInfo[INDEX_MONO_PATH]))
        ret = APC_OK;

    if (bKolorStream && APC_OK == APC_CloseDevice(CEYSDDeviceManager::GetInstance()->GetEYSD(), m_deviceSelInfo[INDEX_COLOR_PATH1]))
        ret = APC_OK;

    return ret;
}

int CVideoDeviceModel_80363::ClosePreviewView()
{
    if (m_pVideoDeviceController->GetPreviewOptions()->IsPointCloudViewer()) {
        m_pVideoDeviceController->GetControlView()->ClosePointCloud();
    }

    bool bTrackStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK);
    if (bTrackStream){
        m_pVideoDeviceController->GetControlView()->ClosePreviewView(STREAM_TRACK);
    }
    bool bKolorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    if (bKolorStream){
        m_pVideoDeviceController->GetControlView()->ClosePreviewView(STREAM_KOLOR);
    }

    return APC_OK;
}

int CVideoDeviceModel_80363::OpenDevice()
{
    bool bKolorStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    if(bKolorStream) {
        int nFPS = m_pVideoDeviceController->GetPreviewOptions()->GetStreamFPS(STREAM_KOLOR);
        if(APC_OK != APC_OpenDevice2(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                     m_deviceSelInfo[INDEX_COLOR_PATH1],
                                     m_imageData[STREAM_KOLOR].nWidth, m_imageData[STREAM_KOLOR].nHeight,
                                     m_imageData[STREAM_KOLOR].bMJPG,
                                     0, 0,
                                     DEPTH_IMG_NON_TRANSFER,
                                     true, nullptr,
                                     &nFPS,
                                     IMAGE_SN_SYNC)) {
            return APC_OPEN_DEVICE_FAIL;
        }
        m_pVideoDeviceController->GetPreviewOptions()->SetStreamFPS(STREAM_KOLOR, nFPS);
    }

    bool bTrackStream = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK);
    if(bTrackStream) {
        int nFPS = m_pVideoDeviceController->GetPreviewOptions()->GetStreamFPS(STREAM_TRACK);
        if(APC_OK != APC_OpenDevice2(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                     m_deviceSelInfo[INDEX_MONO_PATH],
                                     m_imageData[STREAM_TRACK].nWidth, m_imageData[STREAM_TRACK].nHeight,
                                     m_imageData[STREAM_TRACK].bMJPG,
                                     0, 0,
                                     DEPTH_IMG_NON_TRANSFER,
                                     true, nullptr,
                                     &nFPS,
                                     IMAGE_SN_SYNC)){
            return APC_OPEN_DEVICE_FAIL;
        }

        m_pVideoDeviceController->GetPreviewOptions()->SetStreamFPS(STREAM_TRACK, nFPS);
    }

    return APC_OK;
}

int CVideoDeviceModel_80363::AdjustZDTableByPointCloudInfo(PointCloudInfo& info) {
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
        qDebug("[80363] AdjustZDTableByPointCloudInfo: START - Current PreviewOptions ZRange: %d, %d | Parsed from ZDTable: %d, %d | m_bIsFirstStream=%d",
               nZNear, nZFar, m_zdTableInfo.nZNear, m_zdTableInfo.nZFar, m_bIsFirstStream);

        // First stream (first launch): Apply ColorPaletteConfig with priority
        // Subsequent streams (mode changes): Preserve user-set Z range, don't modify
        if (m_bIsFirstStream) {
            bool bNoCalibration = (m_zdTableInfo.nZNear == USHRT_MAX);

            if (bNoCalibration) {
                qDebug("[80363] AdjustZDTableByPointCloudInfo: FIRST STREAM - NO CALIBRATION detected");
                // Use defaults for ColorPaletteConfig when no calibration data
                m_zdTableInfo.nZNear = 0;
                m_zdTableInfo.nZFar = 1000;
            }

            // Apply ColorPaletteConfig (will use provided values or its own defaults)
            int configuredZNear, configuredZFar;
            ColorPaletteConfig::GetInstance()->ApplyConfig(
                "80363",
                m_zdTableInfo.nZNear,
                nZFar,
                configuredZNear,
                configuredZFar
            );

            m_pVideoDeviceController->GetPreviewOptions()->SetZRange(configuredZNear, configuredZFar);
            m_pVideoDeviceController->AdjustZRange();
            m_pVideoDeviceController->GetPreviewOptions()->SetDefaultZRange(configuredZNear, configuredZFar);
            m_bIsFirstStream = false;

            if (bNoCalibration) {
                return APC_NO_CALIBRATION_LOG;
            }
        } else {
            // Subsequent streams (mode changes): Respect user adjustments
            qDebug("[80363] AdjustZDTableByPointCloudInfo: SUBSEQUENT STREAM - Preserving user ZRange (not applying config): ParsedFromZDTable=%d,%d | PreviewOptions=%d,%d",
                   m_zdTableInfo.nZNear, m_zdTableInfo.nZFar, nZNear, nZFar);
            // Do nothing - keep current PreviewOptions values
        }
    }

    return APC_OK;
}

CVideoDeviceModel::ImageData &CVideoDeviceModel_80363::GetColorImageData() {
    return m_imageData[STREAM_KOLOR];
}

CVideoDeviceModel::ImageData &CVideoDeviceModel_80363::GetDepthImageData() {
    return m_imageData[STREAM_TRACK];
}
/**
 * Depending on current mode iK_Index in the database to generate the point cloud info and ZDTable.
 * @return APC_OK
 */
int CVideoDeviceModel_80363::PreparePointCloudInfo() {
    uint8_t currentModeFileIndex = GetVideoDeviceController()->GetModeConfigOptions()->GetCurrentModeInfo().iK_Index;
    GetRectifyLogData(GetRectifyDefaultDeviceSelInfo()->index, currentModeFileIndex, &m_rectifyLogData);
    GetPointCloudInfo(&m_rectifyLogData, m_pointCloudInfo, GetColorImageData(), GetDepthImageData());
    int ret = AdjustZDTableByPointCloudInfo(m_pointCloudInfo);
    if (ret == APC_NO_CALIBRATION_LOG) CMessageManager::ShowMessage("Seems you did not calibrate the camera.");
    return APC_OK;
}

int CVideoDeviceModel_80363::GetImage(STREAM_TYPE type) {
    auto isInterleaveMode = IsInterleaveMode();
    int ret = APC_Init_Fail;
    unsigned long int nImageSize = 0;
    int nSerial = EOF;
    DEVSELINFO *deviceSelInfo = nullptr;

    if (isInterleaveMode) {
        deviceSelInfo = m_deviceSelInfo[INDEX_COLOR_PATH1];

        ret = APC_GetImage(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                           deviceSelInfo,
                           mImageBufferTempForInterleaveMode.data(),
                           &nImageSize,
                           &nSerial,
                           0);

        type = nSerial % 2 == 1 ? STREAM_KOLOR : STREAM_TRACK;
        m_imageData[type].imageBuffer.swap(mImageBufferTempForInterleaveMode);

        if (type == STREAM_KOLOR) {
            if (m_bIsInterleaveStreamStarting && (nSerial == 1 || nSerial == 3)) {
                return APC_OK;
            } else {
                if (m_bIsInterleaveStreamStarting) {
                    m_bIsInterleaveStreamStarting = false;
                }
            }
        }
    } else {
        deviceSelInfo = STREAM_KOLOR == type ? m_deviceSelInfo[INDEX_COLOR_PATH1] : m_deviceSelInfo[INDEX_MONO_PATH];

        ret = APC_Init_Fail;
        bool skipProcessing = false;
        if (type == STREAM_KOLOR && IsCurrentModeDepthOnly()) {
            skipProcessing = true;
        }

        ret = APC_GetImage(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                           deviceSelInfo,
                           &m_imageData[type].imageBuffer[0],
                           &nImageSize,
                           &nSerial,
                           0);

        /**
         * eSP936 depth only need to stream-on color path 1, but we don't show on the screen.
         * So we decide to cancel the callback to the view.
         */
        if (skipProcessing) {
            return APC_OK;
        }
    }

    if (APC_OK != ret) {
        return ret;
    }

    // Process image normally otherwise
    return ProcessImage(type, nImageSize, nSerial);
}

int CVideoDeviceModel_80363::ProcessImage(STREAM_TYPE streamType, int nImageSize, int nSerialNumber) {
    if (m_pVideoDeviceController &&
        m_pVideoDeviceController->GetPreviewOptions() &&
        m_pVideoDeviceController->GetPreviewOptions()->IsModuleSync() &&
        !IsInterleaveMode()) {
        int fps = m_pVideoDeviceController->GetPreviewOptions()->GetStreamFPS(streamType);
        if (fps > 0) {
            nSerialNumber = TimingCountToFrameCount(nSerialNumber, fps);
        }
    }
    return ProcessImageCallback(streamType, nImageSize, nSerialNumber);
}

/**
 * eSP936 series using mono path (baseIndex + 2) to setup IR due to hardware / firmware condition.
 * APC_GetFWRegister APC_GetCurrentIRValue PDEVSELINFO member index need to be (baseIndex + 2)
 * For example, your baseIndex is color path zero (it is 0), then your mono path index is 2.
 * @return APC_OK if get successful result.
 */
int CVideoDeviceModel_80363::UpdateIR() {
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    int ret;
    ret = APC_GetFWRegister(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH],
                            0xE2, &m_nIRMax,
                            FG_Address_1Byte | FG_Value_1Byte);
    if (APC_OK != ret) {
        return ret;
    }

    ret = APC_GetFWRegister(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH],
                            0xE1, &m_nIRMin,
                            FG_Address_1Byte | FG_Value_1Byte);
    if (APC_OK != ret) {
        return ret;
    }

    ret = APC_GetCurrentIRValue(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                m_deviceSelInfo[INDEX_MONO_PATH], &m_nIRValue);
    if (APC_OK != ret) {
        return ret;
    }

    return APC_OK;
}

int CVideoDeviceModel_80363::SetIRValue(unsigned short nValue)
{
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    int ret;

    if (nValue != 0) {
        RETRY_APC_API(ret, APC_SetIRMode(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], 0x3f)); // 6 bits on for opening both 6 ir
        RETRY_APC_API(ret, APC_SetCurrentIRValue(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], nValue));
    } else {
        RETRY_APC_API(ret, APC_SetCurrentIRValue(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], nValue));
    }

    UpdateIR();

    return APC_OK;
}

int CVideoDeviceModel_80363::StopStreaming(bool bRestart) {
    ChangeState(OPENED);

    // MCS: Do NOT reset FW registers (bit[0], bit[1]) on stop.
    // Let FW state persist so it stays in sync with UI checkboxes.
    // Only reset the one-shot "force sync fired" flag so Master checkbox
    // becomes clickable again on next preview session.
    m_pVideoDeviceController->GetPreviewOptions()->SetModuleSyncMaster(false);

    if (m_pVideoDeviceController->GetPreviewOptions()->IsIMUSyncWithFrame()) {
        m_pVideoDeviceController->StopIMUSyncWithFrame();
    }

    StopStreamingTask();
    if (m_pVideoDeviceController->GetPreviewOptions()->IsPointCloudViewer()) {
        StopFrameGrabber();
    }

    SetIRValue(0);
    sleep(3);

    if (!bRestart) ClosePreviewView();

    CloseDevice();
    return APC_OK;
}

int CVideoDeviceModel_80363::StopStreamingTask() {
    for (CTaskInfo *pTask : m_taskInfoStorage) {
        CThreadWorkerManage::GetInstance()->RemoveTask(pTask);
    }

    m_taskInfoStorage.clear();

    if (InterleaveModeSupport()) {
        if (IsInterleaveMode()) {
            m_bIsInterleaveStreamStarting = false;

            // Re-enable all camera properties that were disabled during interleave mode
            // by restoring their valid state to true
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::LOW_LIGHT_COMPENSATION, true);
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::AUTO_WHITE_BLANCE, true);
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::WHITE_BLANCE_TEMPERATURE, true);
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::AUTO_EXPOSURE, true);
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::EXPOSURE_TIME, true);
            m_cameraPropertyModel[0]->SetCameraPropertyValid(CCameraPropertyModel::LIGHT_SOURCE, true);

            // Restore the previous value
            m_cameraPropertyModel[0]->SetCameraPropertyValue(CCameraPropertyModel::LOW_LIGHT_COMPENSATION, m_bPrevLowLightValue);

            SetInterleaveModeEnable(false);
        }
    }

    m_bIsNeedCropLeftImage = false;

    return APC_OK;
}

int CVideoDeviceModel_80363::ExtendIR(bool bEnable)
{
    int ret;
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();

    bool bDisableIR = (0 == m_nIRValue);
    if (bDisableIR) {
        RETRY_APC_API(ret, APC_SetIRMode(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], 0x03)); // 2 bits on for opening both 2 ir
    }

    RETRY_APC_API(ret, APC_SetIRMaxValue(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], bEnable ? kExtendIRMax : kDefaultIRMax));

    if (bDisableIR) {
        RETRY_APC_API(ret, APC_SetIRMode(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], 0x00)); // turn off ir
    }

    if (ret != APC_OK) return ret;

    ret = UpdateIR();

    return ret;
}

bool CVideoDeviceModel_80363::IsInterleaveMode() {
    if (!InterleaveModeSupport()) return false;

    bool bIsInterLeave = false;

    bool bKolorStream = GetVideoDeviceController()->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR);
    bool bTrackStream = GetVideoDeviceController()->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK);

    if (!bKolorStream || !bTrackStream) {
        return bIsInterLeave;
    }

    bIsInterLeave = GetVideoDeviceController()->GetModeConfigOptions()->GetCurrentModeInfo().iInterLeaveModeFPS != 0;

    return bIsInterLeave;
}

int CVideoDeviceModel_80363::SetInterleaveModeEnable(bool bEnable) {
    if (!InterleaveModeSupport()) return APC_NotSupport;

    int ret;
    RETRY_APC_API(ret, APC_EnableInterleave(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                            GetDeviceSelInfo()[INDEX_MONO_PATH],
                                            bEnable));

    return ret;
}

int CVideoDeviceModel_80363::PrepareOpenDevice()
{
    // YX80363: Initialize firmware register 0xE2 (IR max) to 12
    // (firmware default is 6, but we want 12 as our base state for extended IR feature)
    void *pEYSDI = CEYSDDeviceManager::GetInstance()->GetEYSD();
    if (pEYSDI) {
        int ret = APC_OK;
        RETRY_APC_API(ret, APC_SetIRMaxValue(pEYSDI, m_deviceSelInfo[INDEX_MONO_PATH], kDefaultIRMax));
        if (ret != APC_OK) {
            qDebug("[80363] Warning: Failed to set FW register 0xE2, ret=%d", ret);
        }
    }

    SetIRValue(m_pVideoDeviceController->GetPreviewOptions()->GetIRLevel());

    // Re-apply MCS timing count from UI after device reopen (FW register resets on CloseDevice)
    SetModuleSyncTimingCount(m_pVideoDeviceController->GetPreviewOptions()->IsModuleSync());


    if (InterleaveModeSupport() &&
        APC_OK != SetInterleaveModeEnable(IsInterleaveMode())) {
        CTaskInfo *pInfo = CTaskInfoManager::GetInstance()->RequestTaskInfo(CTaskInfo::VIDEO_INTERLEAVE_MODE, this);
        CThreadWorkerManage::GetInstance()->AddTask(pInfo);
    }

    auto PrepareImageData = [&](STREAM_TYPE type){
        bool bStreamEnable = m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(type);
        if(!bStreamEnable){
            return;
        }

        std::vector<APC_STREAM_INFO> streamInfo = GetStreamInfoList(type);
        int index = m_pVideoDeviceController->GetPreviewOptions()->GetStreamIndex(type);
        m_imageData[type].nWidth = streamInfo[index].nWidth;
        m_imageData[type].nHeight  = streamInfo[index].nHeight;
        m_imageData[type].bMJPG = streamInfo[index].bFormatMJPG;
        m_imageData[type].depthDataType = GetDepthDataType();

        if (type == STREAM_KOLOR) {
            m_imageData[type].imageDataType = m_imageData[type].bMJPG ? APCImageType::COLOR_MJPG:
                    APCImageType::COLOR_YUY2;

        } else if (type == STREAM_TRACK) {
            m_imageData[type].imageDataType = ESP936VideoModeToImageType(m_imageData[type].depthDataType);
        }

        unsigned short nBytePerPixel = 2;
        unsigned int nBufferSize = m_imageData[type].nWidth * m_imageData[type].nHeight * nBytePerPixel;
        if (m_imageData[type].imageBuffer.size() != nBufferSize){
            m_imageData[type].imageBuffer.resize(nBufferSize);
            m_imageData[type].processedBuffer.resize(nBufferSize);
        }
        memset(&m_imageData[type].imageBuffer[0], 0, sizeof(nBufferSize));

    };

    PrepareImageData(STREAM_KOLOR);
    PrepareImageData(STREAM_TRACK);

    /**
     * Temporary buffer for GRAPE/ORANGE interleave mode because we don't know the image is color or depth until we get it.
     */
    if (m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_KOLOR)) {
        mImageBufferTempForInterleaveMode.resize(
                m_imageData[STREAM_KOLOR].nWidth * m_imageData[STREAM_KOLOR].nHeight * 2 /* Max BPP depth or color */);
    }


    return APC_OK;
}

bool CVideoDeviceModel_80363::InterleaveModeSupport() {
    return true;
}

int CVideoDeviceModel_80363::InitStreamInfoList(){
    m_streamInfo[STREAM_TRACK].resize(MAX_STREAM_INFO_COUNT, {0, 0, false});
    int ret;
    RETRY_APC_API(ret, APC_GetDeviceResolutionList(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                                   m_deviceSelInfo[INDEX_MONO_PATH],
                                                   MAX_STREAM_INFO_COUNT, &m_streamInfo[STREAM_TRACK][0],
                                                   MAX_STREAM_INFO_COUNT, nullptr));

    if (APC_OK != ret) return ret;

    auto it = m_streamInfo[STREAM_TRACK].begin();
    for ( ; it != m_streamInfo[STREAM_TRACK].end() ; ++it){
        if (0 == (*it).nWidth){
            break;
        }
    }
    m_streamInfo[STREAM_TRACK].erase(it, m_streamInfo[STREAM_TRACK].end());
    m_streamInfo[STREAM_TRACK].shrink_to_fit();
    return CVideoDeviceModel_Kolor::InitStreamInfoList();
}

int CVideoDeviceModel_80363::AdjustZDTableIndex(int &nIndex) {
    if (!m_pVideoDeviceController->GetPreviewOptions()->IsStreamEnable(STREAM_TRACK)){
        return APC_OK;
    }

    nIndex = GetVideoDeviceController()->GetModeConfigOptions()->GetCurrentModeInfo().iT_Index[0];

    return APC_OK;
}

std::vector<CloudPoint> CVideoDeviceModel_80363::GeneratePointCloud(std::vector<unsigned char> &depthData, std::vector<unsigned char> &colorData,
                                                                   unsigned short nDepthWidth, unsigned short nDepthHeight,
                                                                   unsigned short nColorWidth, unsigned short nColorHeight,
                                                                   eSPCtrl_RectLogData rectifyLogData,
                                                                   APCImageType::Value depthImageType,
                                                                   int nZNear, int nZFar,
                                                                   bool bUsePlyFilter,
                                                                   std::vector<float> imgFloatBufOut) {
    auto pixelCount = nColorWidth * nColorHeight;
    std::vector<unsigned char> outCloudColor(pixelCount * 3 /*R G B*/);
    std::vector<float> outCloudDepth(pixelCount * 3 /* X Y Z*/);
    std::vector<CloudPoint> cloudPoints(pixelCount);
    int ret = APC_GetPointCloud(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                                m_deviceSelInfo[0],
                                colorData.data(), nColorWidth, nColorHeight,
                                depthData.data(), nDepthWidth, nDepthHeight,
                                &m_pointCloudInfo,
                                &outCloudColor[0],
                                &outCloudDepth[0],
                                (float) nZNear, (float) nZFar);

    // Check if APC_GetPointCloud was successful before processing
    if (ret == APC_OK) {
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t base_idx = i * 3;
            if (std::isnan(outCloudDepth[base_idx + 2]) || outCloudDepth[base_idx + 2] == 0.0f) continue;

            CloudPoint point {
                .x = outCloudDepth[base_idx],
                .y = outCloudDepth[base_idx + 1],
                .z = outCloudDepth[base_idx + 2],
                .r = outCloudColor[base_idx],
                .g = outCloudColor[base_idx + 1],
                .b = outCloudColor[base_idx + 2]
            };

            cloudPoints.push_back(point);
        }
    } else {
        cloudPoints.clear(); // Example: return an empty vector on error
    }

    return cloudPoints;
}

int CVideoDeviceModel_80363::InitDeviceInformation(){
    CVideoDeviceModel_Kolor::InitDeviceInformation();
    m_deviceInfo.push_back(GetDeviceInformation(m_deviceSelInfo[INDEX_MONO_PATH]));
    return APC_OK;
}

int CVideoDeviceModel_80363::Init()
{
    Reset();
    InitDeviceSelInfo();
    InitDeviceInformation();
    InitStreamInfoList();
    InitUsbType();
    InitCameraproperty();
    InitIMU();
    Update();
    if (m_pVideoDeviceController) m_pVideoDeviceController->Init();
    return DataVerification();
}

int CVideoDeviceModel_80363::InitDeviceSelInfo() {
    CVideoDeviceModel_Kolor::InitDeviceSelInfo();

    if(m_deviceSelInfo.empty()) return APC_NullPtr;

    DEVSELINFO *pDevSelfInfo = new DEVSELINFO;
    pDevSelfInfo->index = m_deviceSelInfo[INDEX_COLOR_PATH0]->index + 2;
    m_deviceSelInfo.push_back(std::move(pDevSelfInfo));

    return APC_OK;
}

int CVideoDeviceModel_80363::AddCameraPropertyModels() {
    CCameraPropertyModel *pColorPath1Model = new CCameraPropertyModel("Color_Path1", this,
                                                                      m_deviceSelInfo[INDEX_COLOR_PATH1]);
    m_cameraPropertyModel.push_back(std::move(pColorPath1Model));
    return APC_OK;
}

APCImageType::Value CVideoDeviceModel_80363::ESP936VideoModeToImageType(unsigned short dataType) {
    switch (dataType) {
        // Legacy video modes for FW001 (0x18-0x1B)
        case 0x1a /* Similar to eSP876 APC_DEPTH_DATA_11_BITS */:
        case 0x18 /* Similar to eSP876 APC_DEPTH_DATA_ILM_11_BITS */:
        // New video modes for YX80363 (0x48, 0x4A)
        case 0x48 /* D11 Interleave */:
        case 0x4A /* D11 Non-Interleave */:
            return APCImageType::DEPTH_11BITS;
        // Legacy video modes for FW001 (0x19, 0x1B)
        case 0x1b /* Similar to eSP876 APC_DEPTH_DATA_14_BITS */:
        case 0x19 /* Similar to eSP876 APC_DEPTH_DATA_ILM_14_BITS */:
        // New video modes for YX80363 (0x49, 0x4B)
        case 0x49 /* D14 Interleave */:
        case 0x4B /* D14 Non-Interleave */:
            return APCImageType::DEPTH_14BITS;
        case 0x0:
            return APCImageType::COLOR_YUY2;
        default:
            fprintf(stderr, "CVideoDeviceModel_80363 APCImageType::IMAGE_UNKNOWN\n");
            return APCImageType::IMAGE_UNKNOWN;
    }
}

int CVideoDeviceModel_80363::UpdateFrameGrabberDepthData(CVideoDeviceModel::STREAM_TYPE streamType) {
    const bool bIsDepthPaletteView = PreviewOptions::POINT_CLOUDE_VIEWER_OUTPUT_DEPTH ==
                                     m_pVideoDeviceController->GetPreviewOptions()->GetPointCloudViewOutputFormat();
    CImageDataModel *pDepthImageData = GetPreivewImageDataModel(streamType);

    if (!pDepthImageData) return APC_NullPtr;

    int nBytesPerPixelDepth = 2;

    if (bIsDepthPaletteView) {
        m_pFrameGrabber->SetFrameFormat(FrameGrabber::FRAME_POOL_INDEX_COLOR,
                                        pDepthImageData->GetWidth(), pDepthImageData->GetHeight(),
                                        3);
        m_pFrameGrabber->UpdateFrameData(FrameGrabber::FRAME_POOL_INDEX_COLOR,
                                         pDepthImageData->GetSerialNumber(),
                                         &pDepthImageData->GetRGBData()[0],
                                         pDepthImageData->GetRGBData().size());
    }
    m_pFrameGrabber->SetFrameFormat(FrameGrabber::FRAME_POOL_INDEX_DEPTH,
                                    pDepthImageData->GetWidth(), pDepthImageData->GetHeight(),
                                    nBytesPerPixelDepth);
    m_pFrameGrabber->UpdateFrameData(FrameGrabber::FRAME_POOL_INDEX_DEPTH,
                                     pDepthImageData->GetSerialNumber(),
                                     &pDepthImageData->GetRawData()[0],
                                     pDepthImageData->GetRawData().size());
    return APC_OK;
}

int CVideoDeviceModel_80363::UpdateFrameGrabberData(CVideoDeviceModel::STREAM_TYPE streamType) {
    switch (streamType) {
        case STREAM_COLOR:
        case STREAM_KOLOR: {
            UpdateFrameGrabberColorData(streamType);
            if (IsInterleaveMode()) {
                UpdateFrameGrabberDepthData(STREAM_TRACK);
            }
            break;
        }
        case STREAM_DEPTH:
        case STREAM_TRACK: {
            UpdateFrameGrabberDepthData(streamType);
            break;
        }
        default: return APC_NotSupport;
    }

    return APC_OK;
}

int
CVideoDeviceModel_80363::ProcessFrameGrabberCallback(vector<unsigned char> &bufDepth, int widthDepth, int heightDepth,
                                                     vector<unsigned char> &bufColor, int widthColor, int heightColor,
                                                     int serialNumber) {
    if (!widthDepth || !heightDepth || bufDepth.empty() || !widthColor || !heightColor || bufColor.empty()) {
        return APC_OK;
    }

    size_t nPointCloudSize;
    int ret;
    nPointCloudSize = widthDepth * heightDepth * 3;

    if (m_pointCloudDepth.size() != nPointCloudSize) m_pointCloudDepth.resize(nPointCloudSize);
    else std::fill(m_pointCloudDepth.begin(), m_pointCloudDepth.end(), 0.0f);

    if (m_pointCloudColor.size() != nPointCloudSize) m_pointCloudColor.resize(nPointCloudSize);
    else std::fill(m_pointCloudColor.begin(), m_pointCloudColor.end(), 0);

    FrameGrabberDataTransform(bufDepth, widthDepth, heightDepth,
                              bufColor, widthColor, heightColor,
                              serialNumber);

    int nZNear, nZFar;
    m_pVideoDeviceController->GetPreviewOptions()->GetZRange(nZNear, nZFar);
    float fZNear = (nZNear * 1.0f) > 0 ? nZNear : 0.1f;
    float fZFar = (nZFar * 1.0f) > 0? nZFar : 1000.0f;
    unsigned char *pImgColor = bufColor.empty() ? nullptr : &bufColor[0];

    nPointCloudSize = widthDepth * heightDepth * 3;

    if (m_bIsNeedCropLeftImage) {
        widthColor /= 2;
    }

    ret = APC_GetPointCloud(CEYSDDeviceManager::GetInstance()->GetEYSD(),
                            m_deviceSelInfo[0],
                            pImgColor, widthColor, heightColor,
                            &bufDepth[0], widthDepth, heightDepth,
                            &m_pointCloudInfo,
                            &m_pointCloudColor[0],
                            &m_pointCloudDepth[0],
                            fZNear, fZFar);

    if (APC_OK == ret)
        m_pVideoDeviceController->GetControlView()->PointCloudCallback(m_pointCloudDepth, m_pointCloudColor);

    return APC_OK;
}

int CVideoDeviceModel_80363::UpdateZDTable() {
    return APC_OK;
}

/**
 * Override for eSP936 (80363): calibration data is stored on color path 1.
 */
int CVideoDeviceModel_80363::CopyG1FileToG2(int fileIndex) {
    return CopyG1FileToG2WithDevSelInfo(fileIndex, m_deviceSelInfo[INDEX_COLOR_PATH1]);
}
