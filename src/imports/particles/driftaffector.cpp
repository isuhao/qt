#include "driftaffector.h"
#include "particlesystem.h"

DriftAffector::DriftAffector(QObject *parent) :
    ParticleAffector(parent)
{
}

DriftAffector::~DriftAffector()
{
}

void DriftAffector::reset(int idx)
{
}

bool DriftAffector::affect(ParticleData *data, qreal dt)
{
    if(!m_xDrift && !m_yDrift)
        return false;
    qreal dx = (((qreal)qrand() / (qreal)RAND_MAX) - 0.5) * 2 * m_xDrift * dt;
    qreal dy = (((qreal)qrand() / (qreal)RAND_MAX) - 0.5) * 2 * m_yDrift * dt;
    if(dx)
        data->setInstantaneousSX(data->curSX() + dx);
    if(dy)
        data->setInstantaneousSY(data->curSY() + dy);

    return true;
}