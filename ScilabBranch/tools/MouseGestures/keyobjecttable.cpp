#include "keyobjecttable.h"
#include "GeometricForms.h"
#include "pathcorrector.h"
#include "levenshteindistance.h"

KeyObjectTable::KeyObjectTable(IKeyManager * keyManager)
{
    mKeyManager = keyManager;
}

KeyObjectTable::KeyObjectTable()
{
    mKeyManager = &mMouseMovementManager;
}

KeyObjectItem KeyObjectTable::at(int pos)
{
    return mKeyObjectTable.at(pos);
}

int KeyObjectTable::size()
{
    return mKeyObjectTable.size();
}

Objects KeyObjectTable::getObjects()
{
    Objects objects;
    foreach (KeyObjectItem item, mKeyObjectTable)
    {
        objects.push_back(Object(item.object, item.correctPath));
    }
    return objects;
}

void KeyObjectTable::add(QString const & object, QList<QPoint> const & correctPath)
{
    QString key = mKeyManager->getKey(PathCorrector::getMousePath(correctPath));
    KeyObjectItem keyObjectItem(object, correctPath, key);
    mKeyObjectTable.push_back(keyObjectItem);
}

void KeyObjectTable::add(const Objects &objects)
{
    foreach (Object object, objects)
    {
        add(object.name, object.path);
    }
}

void KeyObjectTable::clear()
{
    mKeyObjectTable.clear();
}

void KeyObjectTable::setKeyManager(IKeyManager * keyManager)
{
    mKeyManager = keyManager;
    for (int i = 0; i < mKeyObjectTable.size(); i++)
    {
        mKeyObjectTable[i].key = mKeyManager->getKey(
                PathCorrector::getMousePath(mKeyObjectTable[i].correctPath));
    }
}

void KeyObjectTable::setPath(QString const & object, QList<QPoint> const & correctPath)
{
    for (int i = 0; i < mKeyObjectTable.size(); i++)
    {
        if (mKeyObjectTable[i].object == object)
        {
            mKeyObjectTable[i].correctPath = correctPath;
            QString key = mKeyManager->getKey(PathCorrector::getMousePath(correctPath));
            mKeyObjectTable[i].key = key;
            return;
        }
    }
    add(object, correctPath);
}

QString KeyObjectTable::getObject(QList<QPoint> const & path)
{
    const float e = 100;
    const float maxKeyDistance = 100;
    float min = e;
    float distance;
    QString key = mKeyManager->getKey(path);
    QString object = "";
    if (key.isEmpty())
        return object;
    foreach (KeyObjectItem item, mKeyObjectTable)
    {
        if (!item.key.isEmpty())
        {
            distance = (float)(LevenshteinDistance::getLevenshteinDistance(item.key, key) * e
                                / std::min(key.size(), item.key.size()));
            if (distance < min && distance < maxKeyDistance)
            {
                min = distance;
                object = item.object;
            }
        }
    }
    return object;
}

QString KeyObjectTable::getKey(QString const & name)
{
    return getItem(name).key;
}

QList<QPoint> KeyObjectTable::getPath(QString const & name)
{
    return getItem(name).correctPath;
}

KeyObjectItem KeyObjectTable::getItem(QString const & name)
{
    foreach (KeyObjectItem item, mKeyObjectTable)
    {
        if (item.object == name)
            return item;
    }
    return KeyObjectItem();
}
