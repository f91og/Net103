#ifndef 103FUNCTION_H
#define 103FUNCTION_H

#include "103struct.h"

class WaveFileData
{
public:
    WaveFileData();
    ~WaveFileData();

public:
     QString CreateWaveFile(int CpuNo,QString FilePath);

private:
    void CreateCfgFile(int CpuNo,QString FileName);
    bool CreateDatFile(QString FileName);
    void CreateHdrFile(QString FileNames);
};

#endif // 103FUNCTION_H
