#include "mbed.h"
#include <string>
#include <vector>

extern "C" 
{
#include "iniparser.h"
}

class SPKSettings {
public:
    enum keyerParameterType {minY = 0, maxY, minU, maxU, minV, maxV};

    int editingKeyerSetIndex;

    SPKSettings()
    {
        editingKeyerSetIndex = -1;
        loadDefaults();
    }
    
    void loadDefaults()
    {
        //// KEYS
        
        keyerParamNames.clear();
        keyerParamSets.clear();
        vector<int> paramSet(6);
        
        paramSet[minY] = 0;
        paramSet[maxY] = 18;
        paramSet[minU] = 128;
        paramSet[maxU] = 129;
        paramSet[minV] = 128;
        paramSet[maxV] = 129;
        keyerParamSets.push_back(paramSet);
        keyerParamNames.push_back("Lumakey");
        
        paramSet[minY] = 30;
        paramSet[maxY] = 35;
        paramSet[minU] = 237;
        paramSet[maxU] = 242;
        paramSet[minV] = 114;
        paramSet[maxV] = 121;
        keyerParamSets.push_back(paramSet);
        keyerParamNames.push_back("Chromakey - Blue");
        
        //// RESOLUTIONS
        
        resolutionNames.clear();
        resolutionIndexes.clear();
        resolutionEDIDIndexes.clear();
        
        resolutionNames.push_back(kTV1ResolutionDescriptionVGA);
        resolutionIndexes.push_back(kTV1ResolutionVGA);
        resolutionEDIDIndexes.push_back(6);
    
        resolutionNames.push_back(kTV1ResolutionDescriptionSVGA);
        resolutionIndexes.push_back(kTV1ResolutionSVGA);
        resolutionEDIDIndexes.push_back(6);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionXGAp60);
        resolutionIndexes.push_back(kTV1ResolutionXGAp60);
        resolutionEDIDIndexes.push_back(6);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionWSXGAPLUSp60);
        resolutionIndexes.push_back(kTV1ResolutionWSXGAPLUSp60);
        resolutionEDIDIndexes.push_back(6);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionWUXGAp60);
        resolutionIndexes.push_back(kTV1ResolutionWUXGAp60);
        resolutionEDIDIndexes.push_back(6);
        
        resolutionNames.push_back(kTV1ResolutionDescription720p60);
        resolutionIndexes.push_back(kTV1Resolution720p60);
        resolutionEDIDIndexes.push_back(5);
        
        resolutionNames.push_back(kTV1ResolutionDescription1080p60);
        resolutionIndexes.push_back(kTV1Resolution1080p60);
        resolutionEDIDIndexes.push_back(5);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionDualHeadSVGAp60);
        resolutionIndexes.push_back(kTV1ResolutionDualHeadSVGAp60);
        resolutionEDIDIndexes.push_back(4);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionDualHeadXGAp60);
        resolutionIndexes.push_back(kTV1ResolutionDualHeadXGAp60);
        resolutionEDIDIndexes.push_back(4);
        
        resolutionNames.push_back(kTV1ResolutionDescriptionTripleHeadVGAp60);
        resolutionIndexes.push_back(kTV1ResolutionTripleHeadVGAp60);
        resolutionEDIDIndexes.push_back(4);   
    }
    
    string keyerParamName (int index)
    {
        // TODO: Bounds check and return out of bounds name
        return keyerParamNames[index];
    }
     
    vector<int>        keyerParamSet(int index)
    {
        return keyerParamSets[index];
    }
    
    int         keyerSetCount()
    {
        return keyerParamSets.size();
    }
    
    int editingKeyerSetValue(keyerParameterType parameter)
    {
        int value = -1;
        if (editingKeyerSetIndex >= 0 && editingKeyerSetIndex < keyerSetCount())
        {
            value = keyerParamSets[editingKeyerSetIndex][parameter];
        }
        return value;
    }
    
    void setEditingKeyerSetValue(keyerParameterType parameter, int value)
    {
        if (editingKeyerSetIndex >= 0 && editingKeyerSetIndex < keyerSetCount())
        {
            keyerParamSets[editingKeyerSetIndex][parameter] = value;
        }
    }
    
    string resolutionName (int index)
    {
        // TODO: Bounds check and return out of bounds name
        return resolutionNames[index];
    }
     
    int32_t     resolutionIndex(int index)
    {
        return resolutionIndexes[index];
    }
    
    int32_t     resolutionEDIDIndex(int index)
    {
        return resolutionEDIDIndexes[index];
    }
    
    int         resolutionsCount()
    {
        return resolutionNames.size();
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
            
            char* const failString = "Failed to read";
            const int failInt = -1;
            const int stringLength = 11;
            
            // Loop through [Key1,2,...,99] sections
            while(keyParamReadOK)
            {
                vector<int> paramSet(6);
                char*       paramName;
                
                char key[stringLength];
        
                snprintf(key, stringLength, "Key%i:Name", counter);
                paramName = iniparser_getstring(settings, key, failString);
                keyParamReadOK = keyParamReadOK && strcmp(paramName, failString);
                       
                snprintf(key, stringLength, "Key%i:MinY", counter);
                paramSet[minY] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[minY] != failInt);

                snprintf(key, stringLength, "Key%i:MaxY", counter);
                paramSet[maxY] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[maxY] != failInt);
                
                snprintf(key, stringLength, "Key%i:MinU", counter);
                paramSet[minU] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[minU] != failInt);
                
                snprintf(key, stringLength, "Key%i:MaxU", counter);
                paramSet[maxU] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[maxU] != failInt);
                
                snprintf(key, stringLength, "Key%i:MinV", counter);
                paramSet[minV] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[minV] != failInt);
                
                snprintf(key, stringLength, "Key%i:MaxV", counter);
                paramSet[maxV] = iniparser_getint(settings, key, failInt);
                keyParamReadOK = keyParamReadOK && (paramSet[maxV] != failInt);
                
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

        // RESOLUTIONS
        {
            int counter = 1;
            
            bool resolutionReadOK = true;
            bool resolutionCleared = false;
            
            char* const failString = "Failed to read";
            const int failInt = -1;
            const int stringLength = 25;
            
            // Loop through [Key1,2,...,99] sections
            while(resolutionReadOK)
            {
                char*   resolutionName;
                int     resolutionIndex;
                int     resolutionEDIDIndex;
                
                char key[stringLength];
        
                snprintf(key, stringLength, "Resolution%i:Name", counter);
                resolutionName = iniparser_getstring(settings, key, failString);
                resolutionReadOK = resolutionReadOK && strcmp(resolutionName, failString);
                       
                snprintf(key, stringLength, "Resolution%i:Number", counter);
                resolutionIndex = iniparser_getint(settings, key, failInt);
                resolutionReadOK = resolutionReadOK && (resolutionIndex != failInt);

                snprintf(key, stringLength, "Resolution%i:EDIDNumber", counter);
                resolutionEDIDIndex = iniparser_getint(settings, key, failInt);
                resolutionReadOK = resolutionReadOK && (resolutionEDIDIndex != failInt);
                
                // If all parameters have been read successfully
                if (resolutionReadOK)
                {
                    // If this is the first time through, clear old values
                    if (!resolutionCleared)
                    {
                        resolutionNames.clear();
                        resolutionIndexes.clear();
                        resolutionEDIDIndexes.clear();
                        resolutionCleared = true;
                    }
                
                    // Apply settings
                    resolutionNames.push_back(resolutionName);
                    resolutionIndexes.push_back(resolutionIndex);
                    resolutionEDIDIndexes.push_back(resolutionEDIDIndex);
                    
                    // We've successfully read a resolution, so should return true;
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
    vector< vector<int> >   keyerParamSets;
    vector<string>          keyerParamNames;
    vector<string>          resolutionNames;
    vector<int32_t>         resolutionIndexes;
    vector<int32_t>         resolutionEDIDIndexes;
};