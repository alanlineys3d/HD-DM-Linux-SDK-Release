#ifndef CVIDEODEVICEMODEL_HYPATIA4_H
#define CVIDEODEVICEMODEL_HYPATIA4_H
#include "CVideoDeviceModel.h"

class CVideoDeviceModel_Hypatia4 : public CVideoDeviceModel
{
public:
    virtual int TransformDepthDataType(int nDepthDataType, bool bRectifyData);
    virtual int PreparePointCloudInfo();
    virtual int AdjustZDTableIndex(int &nIndex);
    friend class CVideoDeviceModelFactory;
    virtual int DefaultVideoMode(){ return 5; }
protected:
    CVideoDeviceModel_Hypatia4(DEVSELINFO *pDeviceSelfInfo);
};

#endif // CVIDEODEVICEMODEL_HYPATIA4_H
