#include "CSelfCalibration2Widget.h"
#include "ui_CSelfCalibration2Widget.h"
#include <iostream>
#include <QMessageBox>
#include <CVideoDeviceController.h>
#include <QDebug>

CSelfCalibration2Widget::CSelfCalibration2Widget(CVideoDeviceController *pVideoDeviceController, QWidget *parent):
        QWidget(parent),
        ui(new Ui::CSelfCalibration2Widget),
        m_pVideoDeviceController(pVideoDeviceController) {
    ui->setupUi(this);
    ui->radio_button_selfk2_runtime_correction->setChecked(true);
    ui->push_btn_selfk2_write_to_flash->setEnabled(false);
    ui->text_label_selfK2_status->setText("");

    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (controller) {
        connect(controller, &CSelfCalibration2Controller::statusTextChanged, this,
                &CSelfCalibration2Widget::onStatusTextChanged);
    }
}

CSelfCalibration2Widget::~CSelfCalibration2Widget() {
    delete ui;
}

void CSelfCalibration2Widget::on_radio_button_selfk2_depth_broken_repair_toggled(bool checked) {
    if (!m_pVideoDeviceController) return;
    
    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (controller && checked) {
        controller->SetMode(CSelfCalibration2Controller::Mode::DEPTH_BROKEN_REPAIR);
    }
}

void CSelfCalibration2Widget::on_radio_button_selfk2_runtime_correction_toggled(bool checked) {
    if (!m_pVideoDeviceController) return;
    
    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (controller && checked) {
        controller->SetMode(CSelfCalibration2Controller::Mode::RUNTIME_CORRECTION);
    }
}

void CSelfCalibration2Widget::on_push_btn_selfk2_reset_clicked() {
    if (!m_pVideoDeviceController) return;
    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (controller) {
        controller->ResetSelfK2();
    }
}

void CSelfCalibration2Widget::on_push_btn_selfk2_run_clicked() {
    if (!m_pVideoDeviceController) {
        qDebug() << "VideoDeviceController is null";
        return;
    }

    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (!controller) {
        qDebug() << "SelfCalibration2Controller is null";
        return;
    }

    if (!controller->IsRunning()) {
        if (!controller->IsDepthStreaming()) {
            QMessageBox::information(this, tr("Alert"), tr("Stream on any depth stream!!"));
            return;
        }
        
        if (controller->RunSelfK2() == 0) {
            setUIRunningState();
        }
    } else {
        controller->StopSelfK2();
        setUIResetState();
    }
}

void CSelfCalibration2Widget::setUIRunningState() {
    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (!controller) return;
    ui->push_btn_selfk2_run->setText(tr("Stop"));
    ui->radio_button_selfk2_runtime_correction->setEnabled(false);
    ui->radio_button_selfk2_depth_broken_repair->setEnabled(false);
    ui->push_btn_selfk2_reset->setEnabled(true);
    if (controller->GetMode() == CSelfCalibration2Controller::Mode::DEPTH_BROKEN_REPAIR) {
        ui->push_btn_selfk2_write_to_flash->setEnabled(true);
    }
}

void CSelfCalibration2Widget::setUIResetState() {
    ui->push_btn_selfk2_run->setText(tr("Run"));
    ui->radio_button_selfk2_runtime_correction->setEnabled(true);
    ui->radio_button_selfk2_depth_broken_repair->setEnabled(true);
    ui->push_btn_selfk2_reset->setEnabled(false);
    ui->push_btn_selfk2_write_to_flash->setEnabled(false);
}

void CSelfCalibration2Widget::on_push_btn_selfk2_write_to_flash_clicked() {
    if (!m_pVideoDeviceController) return;
    
    auto controller = m_pVideoDeviceController->GetSelfCalibration2Controller();
    if (!controller) return;
    auto cyToBeWritten = controller->GetCurrentCompCy();
    QString message = tr("Write comp_cy: %1 to flash?").arg(cyToBeWritten, 0, 'f', 6);
    if (QMessageBox::question(this, tr("Confirm"), message,
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (controller->WriteToFlash(cyToBeWritten)) {
            QMessageBox::information(this, tr("Alert"), tr("Parameter saved in the flash"));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Write data into flash failed"));
        }
    }
}

void CSelfCalibration2Widget::UpdateSelf() {
    CEYSDUIView::UpdateSelf();
}

void CSelfCalibration2Widget::onStatusTextChanged(const QString &text)
{
    ui->text_label_selfK2_status->setText(text);
}
