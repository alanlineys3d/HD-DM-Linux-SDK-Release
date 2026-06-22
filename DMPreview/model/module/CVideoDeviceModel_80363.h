#ifndef CVIDEODEVICEMODEL_80363_H
#define CVIDEODEVICEMODEL_80363_H
#include "CVideoDeviceModel_Kolor.h"
class CVideoDeviceModel_80363 : public CVideoDeviceModel_Kolor
{
public:
    static constexpr unsigned char INDEX_COLOR_PATH0 = 0;
    static constexpr unsigned char INDEX_COLOR_PATH1 = 1;
    static constexpr unsigned char INDEX_MONO_PATH   = 2;
    static constexpr uint16_t kDefaultIRMax = 12;
    static constexpr uint16_t kExtendIRMax = 15;
    static APCImageType::Value ESP936VideoModeToImageType(unsigned short dataType);
    virtual bool IsStreamAvailable();
    virtual int AddCameraPropertyModels();
    virtual int GetPointCloudInfo(eSPCtrl_RectLogData *pRectifyLogData, PointCloudInfo &pointCloudInfo,
                                  ImageData colorImageData, ImageData depthImageData);
    virtual int CloseDevice();
    virtual int ExtendIR(bool bEnable);
    virtual int UpdateIR();
    virtual int SetIRValue(unsigned short nValue);
    virtual int GetImage(STREAM_TYPE type);
    virtual int ClosePreviewView();
    friend class CVideoDeviceModelFactory;
    virtual int OpenDevice();
    virtual int InitStreamInfoList();
    virtual int SetDepthDataType(int nDepthDataType);
    virtual int UpdateDepthDataType();
    virtual int PrepareOpenDevice();
    virtual int StartStreamingTask();
    virtual int PreparePointCloudInfo();
    virtual int AdjustZDTableIndex(int &nIndex);
    virtual std::vector<CloudPoint> GeneratePointCloud(std::vector<unsigned char> &depthData,
                                                       std::vector<unsigned char> &colorData,
                                                       unsigned short nDepthWidth,
                                                       unsigned short nDepthHeight,
                                                       unsigned short nColorWidth,
                                                       unsigned short nColorHeight,
                                                       eSPCtrl_RectLogData rectifyLogData,
                                                       APCImageType::Value depthImageType,
                                                       int nZNear, int nZFar,
                                                       bool bUsePlyFilter = false,
                                                       std::vector<float> imgFloatBufOut = {});
    /**
     * Rectify table stored in Color PATH1.
     * @return
     */
    virtual DEVSELINFO* GetRectifyDefaultDeviceSelInfo () { return GetDeviceSelInfo()[INDEX_COLOR_PATH1]; }
    virtual int GetCurrentTemperature(float& temperature);
    virtual int InitDeviceSelInfo();
    virtual int Init();
    virtual int InitDeviceInformation();
    virtual bool IsStreamSupport(STREAM_TYPE type);
    virtual int StopStreaming(bool bRestart);

    virtual bool ModuleSyncSupport() override;
    virtual int  ModuleSync() override;
    virtual bool GetModuleSyncMasterCapability() override;
    virtual int  SetModuleSyncForceSync(bool bEnable) override;
    virtual int  SetModuleSyncTimingCount(bool bEnable) override;

    virtual bool InterleaveModeSupport();
    virtual bool IsInterleaveMode();
    virtual int SetInterleaveModeEnable(bool bEnable);
    virtual ImageData& GetColorImageData();
    virtual ImageData& GetDepthImageData();
    virtual bool IsESP936Series();
    virtual APCImageType::Value GetDepthImageType();
    std::vector<STREAM_TYPE> GetDepthAccuracyStreamTypes() override;
    virtual bool IsCurrentModeDepthOnly();
    bool IsRectifyData() override;
    int ProcessImage(STREAM_TYPE streamType, int nImageSize, int nSerialNumber) override;
    std::vector<unsigned char> mImageBufferTempForInterleaveMode;

    int ProcessFrameGrabberCallback(vector<unsigned char> &bufDepth, int widthDepth, int heightDepth,
                                    vector<unsigned char> &bufColor, int widthColor, int heightColor,
                                    int serialNumber) override;

    int StopStreamingTask() override;
    virtual bool IsAETargetSupport() override { return false; }
    virtual int CopyG1FileToG2(int fileIndex) override;

protected:
    CVideoDeviceModel_80363(DEVSELINFO *pDeviceSelfInfo);

    int UpdateFrameGrabberData(STREAM_TYPE streamType) override;

    int UpdateFrameGrabberDepthData(STREAM_TYPE streamType) override;
    int UpdateZDTable() override;
private:
    void UpdateImageProcessor();
    int AdjustZDTableByPointCloudInfo(PointCloudInfo &info);
    bool m_bIsInterleaveStreamStarting;
    bool m_bIsFirstStream = true;

    static inline int TimingCountToFrameCount(int timingCountUs, int fps) {
        uint64_t us = static_cast<uint64_t>(timingCountUs & 0x7FFFFFFF);
        return static_cast<int>((us * fps + 500000) / 1000000);
    }
};

#endif // CVIDEODEVICEMODEL_80363_H
