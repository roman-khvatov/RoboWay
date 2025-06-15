#pragma once

#include <QtWidgets>
#include <QtWidgets/QMainWindow>
#include <QGraphicsEllipseItem>
#include <QWaitCondition>
#include <QTimer>

#include "ui_tetrisemulator.h"

#include "../common/interface.h"

class TetrisEmulator : public QMainWindow
{
    Q_OBJECT

    QGraphicsEllipseItem* field[8][16];

    QGraphicsScene scene;

    uint8_t last_keys = 0;
    uint8_t active_keys = 0;

    QWaitCondition cv;
    QMutex mtx;
    QTimer tmr;

    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;

    void draw_pixels();

    int x=0, y=0;
public:
    TetrisEmulator(QWidget *parent = nullptr);
    ~TetrisEmulator();

    uint8_t read_key();
    void clr_keys(uint8_t keys) {active_keys &= ~keys;}

private:
    Ui::TetrisEmulatorClass ui;
};

