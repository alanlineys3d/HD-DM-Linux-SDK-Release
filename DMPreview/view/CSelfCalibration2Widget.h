#ifndef CSELFCALIBRATION2WIDGET_H
#define CSELFCALIBRATION2WIDGET_H

#include <QWidget>
#include <QTimer>
#include <mutex>
#include "CEYSDUIView.h"

namespace Ui {
class CSelfCalibration2Widget;
}

class CSelfCalibration2Widget : public QWidget, CEYSDUIView
{
    Q_OBJECT

public:
    explicit CSelfCalibration2Widget(CVideoDeviceController *pVideoDeviceController,
                                     QWidget *parent = nullptr);
    ~CSelfCalibration2Widget();

    virtual void UpdateSelf();

private slots:
    void on_radio_button_selfk2_depth_broken_repair_toggled(bool checked);
    void on_radio_button_selfk2_runtime_correction_toggled(bool checked);
    void on_push_btn_selfk2_reset_clicked();
    void on_push_btn_selfk2_run_clicked();
    void on_push_btn_selfk2_write_to_flash_clicked();
    void onStatusTextChanged(const QString &text);

private:
    void UpdateUI();

private:
    Ui::CSelfCalibration2Widget *ui;
    CVideoDeviceController *m_pVideoDeviceController;
    void setUIResetState();
    void setUIRunningState();
};

#endif // CSELFCALIBRATION2WIDGET_H
