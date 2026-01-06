#ifndef CVIDEODEVICEMODEL_HYPATIA2_H
#define CVIDEODEVICEMODEL_HYPATIA2_H
#include "CVideoDeviceModel.h"

class CVideoDeviceModel_Hypatia2 : public CVideoDeviceModel
{
public:
    virtual int TransformDepthDataType(int nDepthDataType, bool bRectifyData);
    virtual int PreparePointCloudInfo();
    virtual int AdjustZDTableIndex(int &nIndex);
    friend class CVideoDeviceModelFactory;
    virtual int DefaultVideoMode(){ return 5; }
protected:
    CVideoDeviceModel_Hypatia2(DEVSELINFO *pDeviceSelfInfo);
private:
    int AdjustZDTableByPointCloudInfo(PointCloudInfo& info);
};

#endif // CVIDEODEVICEMODEL_HYPATIA2_H
