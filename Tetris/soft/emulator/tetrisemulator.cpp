#include <QThread>

#include "tetrisemulator.h"

static TetrisEmulator* root;

TetrisEmulator::TetrisEmulator(QWidget *parent)
    : QMainWindow(parent)
{
    root = this;
    ui.setupUi(this);

    for(int x=0; x<8; ++x)
        for(int y=0; y<16; ++y)
            field[x][y] = scene.addEllipse(QRect(x*25, y*25, 22, 22), QPen(QColorConstants::Black), QBrush(QColorConstants::LightGray));

    ui.main_pane->setScene(&scene);
    ui.main_pane->show();

    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    tmr.setTimerType(Qt::PreciseTimer);
    tmr.callOnTimeout(
        [this]() 
        {
            draw_pixels();
            cv.notify_all();
        }
    );
    tmr.start(tick_time);

    QThread::create([]() {entry();})->start();
}

TetrisEmulator::~TetrisEmulator()
{}

void TetrisEmulator::draw_pixels()
{
    uint8_t* p1=pixs.br1;
    uint8_t* p2=pixs.br2;

    for (int y = 0; y < 16; ++y)
    {
        uint8_t px1 = *p1++;
        uint8_t px2 = *p2++;
        for (int x = 0; x < 8; ++x)
        {
            auto color = QColorConstants::LightGray;
            switch ((px1 & 1) + (px2 & 1) * 2)
            {
                case 0: break;
                case 1: color = QColor(255 / 3, 0, 0); break;
                case 2: color = QColor(255*2 / 3, 0, 0); break;
                case 3: color = QColor(255, 0, 0); break;
            }
            field[x][y]->setBrush(color);
            px1 >>= 1;
            px2 >>= 1;
        }
    }
}

static uint8_t key2key(int key)
{
    switch (key)
    {
        case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Clear: return K_Hit;
        case Qt::Key_Left: return K_Left;
        case Qt::Key_Up: return K_Up;
        case Qt::Key_Right: return K_Right;
        case Qt::Key_Down: return K_Down;
        case Qt::Key_1: return K_1;
        case Qt::Key_2: return K_2;
        case Qt::Key_3: return K_3;
        default: return 0;
    }
}

void TetrisEmulator::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    uint8_t new_key = key2key(event->key());
    if (!new_key) return;
    if (~last_keys & new_key) // Press some
    {
        last_keys |= new_key;
        active_keys |= new_key;
    }
}

void TetrisEmulator::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    uint8_t new_key = key2key(event->key());
    last_keys &= ~new_key;
    active_keys &= ~new_key;
}

uint8_t TetrisEmulator::read_key()
{
    mtx.lock();
    cv.wait(&mtx);
    uint8_t result = active_keys;
    mtx.unlock();
    return result;
}

uint8_t read_key() {return root->read_key();}
void clr_keys(uint8_t keys) {root->clr_keys(keys);}

uint32_t get_random()
{
    return QRandomGenerator::global()->generate();
}
