// Parameter Set: minY, maxY, minU, maxU, minV, maxV
#define kKeyerParam1Name    "Lumakey"
#define kKeyerParam1Set     0, 18, 128, 129, 128, 129
#define kKeyerParam2Name    "Chromakey - Blue"
#define kKeyerParam2Set     30, 35, 237, 242, 114, 121

#include "mbed.h"
#include <string>
#include <vector>

// CRAZY: When this file is used in a test program, doesn't need the extern to compile, and works perfectly.
// When this file is used in SPK-DVIMXR, requires the extern to compile and crashes on iniparser_load
extern "C" 
{
#include "iniparser.h"
}

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
    
    string keyerParamName (int index)
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
    
    bool        load(string filename)
    {
        bool success = false;

        local = new LocalFileSystem("local");
        string filePath("/local/");
        filePath += filename;

        dictionary* settings = iniparser_load(filePath.c_str());
            
        // KEYER
        {
            int counter = 1;
            
            bool keyParamReadOK = true;
            bool keyParamCleared = false;
            
            char* failString = "Failed to read";
            int failInt = -1;
            
            // Loop through [Key1,2,...,99] sections
            while(keyParamReadOK)
            {
                int     paramSet[6];
                char*   paramName;
                
                char key[11];
        
                sprintf(key, "Key%i:Name", counter);
                paramName = iniparser_getstring(settings, key, failString);
                keyParamReadOK = keyParamReadOK && strcmp(paramName, failString);
                       
                sprintf(key, "Key%i:MinY", counter);
                paramSet[0] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[0] != failInt);

                sprintf(key, "Key%i:MaxY", counter);
                paramSet[1] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[1] != failInt);
                
                sprintf(key, "Key%i:MinU", counter);
                paramSet[2] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[2] != failInt);
                
                sprintf(key, "Key%i:MaxU", counter);
                paramSet[3] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[3] != failInt);
                
                sprintf(key, "Key%i:MinV", counter);
                paramSet[4] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[4] != failInt);
                
                sprintf(key, "Key%i:MaxV", counter);
                paramSet[5] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[5] != failInt);
                
                // If all parameters have been read successfully
                if (keyParamReadOK)
                {

                    // If this is the first time through, clear old values
                    if (!keyParamCleared)
                    {
                        keyerParamNames.clear();
                        keyerParamSets.clear();
                        keyParamCleared = true;
                    }
                
                    // Apply settings
                    keyerParamNames.push_back(paramName);
                    keyerParamSets.push_back(paramSet);
                    
                    // We've successfully read a keyer param set, so should return true;
                    success = true;
                }
                
                counter++;
            }
        }        

        iniparser_freedict(settings);
        
        delete local;
        
        return success;
    }
    
protected:
    LocalFileSystem *local;
    vector<int*> keyerParamSets;
    vector<string> keyerParamNames;
};
