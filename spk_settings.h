// Parameter Set: minY, maxY, minU, maxU, minV, maxV
#define kKeyerParam1Name    "Lumakey"
#define kKeyerParam1Set     0, 18, 128, 129, 128, 129
#define kKeyerParam2Name    "Chromakey - Blue"
#define kKeyerParam2Set     30, 35, 237, 242, 114, 121

#include "mbed.h"

class SPKSettings {
public:
    SPKSettings()
    {
        int paramSet1[6] = {kKeyerParam1Set};
        keyerParamNames.push_back(kKeyerParam1Name);
        keyerParamSets.push_back(paramSet1);
        
        int paramSet2[6] = {kKeyerParam2Set};
        keyerParamNames.push_back(kKeyerParam2Name);
        keyerParamSets.push_back(paramSet2);
    }
    
    std::string keyerParamName (int index)
    {
        // TODO: Bounds check and return out of bounds name
        return keyerParamNames[index];
    }
     
    int*        keyerParamSet(int index)
    {
        return keyerParamSets[index];
    }
    
    int         keyerSetCount()
    {
        return keyerParamSets.size();
    }
    
    bool        load(std::string filename)
    {
        bool success = false;
        
        // TODO!
        
        return success;
    }
    
protected:
    LocalFileSystem *local;
    std::vector<int*> keyerParamSets;
    std::vector<std::string> keyerParamNames;
};
