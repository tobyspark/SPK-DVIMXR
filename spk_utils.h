#include <string>
#include <vector>

class SPKIndexInRange {
public:
    void operator = (int newIndex) {
        set(newIndex);
    }
    
    void operator ++ (int) {
        if (idx == max) idx = wrap ? min : max;
        else idx++;
    }
    
    void operator -- (int) {
        if (idx == min) idx = wrap ? max : min;
        else idx--;
    }
    
    void set (int newIndex) {
        if (newIndex > max) idx = max;
        else if (newIndex < min) idx = min;
        else idx = newIndex;
    }
    
    void set (int newMin, int newMax, int newIndex = 0, bool newWrap = false) {
        min = newMin;
        max = newMax;
        wrap = newWrap;
        set(newIndex);
    }
    
    void setMax(int newMax) {
        max = newMax;
    }
    
    SPKIndexInRange () {
        min = 0;
        max = 1;
        wrap = true;
        idx = 0;
    }
    
    SPKIndexInRange (int newMin, int newMax, int newIndex = 0, bool newWrap = false) {
        set(newMin, newMax, newIndex, newWrap);
    }
    
    int index() {
        return idx;
    }
    
private:
    int idx;
    int min, max;
    bool wrap;
};

enum SPKMenuType { menuOfMenus, payload };

class SPKMenu {
public:
    SPKMenu() {
        selected.set(0, 0, 0, true);
    }
    
    virtual SPKMenuType type(void) = 0;
    
    std::string title;
    
    void operator = (int newIndex) {
        selected = newIndex;
    }
    
    void operator ++ () {
        selected++;
    }
    
    void operator -- () {
        selected--;
    }
    
    void addMenuItem (std::string menuText) {
        text.push_back(menuText);
        selected.setMax(text.size()-1);
    }
    
    int selectedIndex() {
        return selected.index();
    }
    
    std::string  selectedString() {
        return text[selected.index()];
    }
        
protected:
    SPKIndexInRange selected;
    std::vector<std::string> text;
};

class SPKMenuOfMenus: public SPKMenu {
public:
    SPKMenuOfMenus() : SPKMenu() {}
    
    virtual SPKMenuType type() {
        return menuOfMenus;
    }
    
    void addMenuItem(SPKMenu* menu) {
        SPKMenu::addMenuItem(menu->title);
        payload.push_back(menu);
    }
    
    SPKMenu* selectedMenu() {
        return payload[selected.index()];
    }
        
private:
    vector<SPKMenu*> payload;
};

class SPKMenuPayload: public SPKMenu {
public:
    SPKMenuPayload() : SPKMenu() {
        text.push_back("Cancel");
        payload1.push_back(0);
        payload2.push_back(0);
    }
    
    virtual SPKMenuType type() {
        return payload;
    }
    
    void addMenuItem(std::string menuText, int32_t menuPayload1, int32_t menuPayload2) {
        SPKMenu::addMenuItem(menuText);
        payload1.push_back(menuPayload1);
        payload2.push_back(menuPayload2);
    }
    
    int32_t selectedPayload1() {
        return payload1[selected.index()];
    }
    
    int32_t selectedPayload2() {
        return payload2[selected.index()];
    }
        
private:
    vector<int32_t> payload1;
    vector<int32_t> payload2;
};

class SPKSign {
public:
    SPKSign(PinName signWrite, PinName signError) {
        writeDO = new DigitalOut(signWrite);
        errorDO = new DigitalOut(signError);
    }
    
    ~SPKSign() {
        delete writeDO;
        delete errorDO;
    }
    
    void serialWrite() {
        signWriteTimeout.detach();
        signWriteTimeout.attach(this, &SPKSign::writeOff, 0.25);
        *writeDO = 1;
    }
    
    void serialError() {
        signErrorTimeout.detach();
        signErrorTimeout.attach(this, &SPKSign::errorOff, 0.25);
        *errorDO = 1;   
    }

private:
    void writeOff() {
        *writeDO = 0;
    }
    
    void errorOff() {
        *errorDO = 0;
    }
    
    DigitalOut *writeDO;
    DigitalOut *errorDO;
    Timeout signWriteTimeout;
    Timeout signErrorTimeout;
};



