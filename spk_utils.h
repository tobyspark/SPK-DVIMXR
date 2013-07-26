// *SPARK D-FUSER
// A project by Toby Harris
// Copyright *spark audio-visual 2012
//
// SPK_UTILS provides utility classes for the main SPK-DVIMXR codebase, most significantly the menu system.

#include <string>
#include <vector>
#include <queue>

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

class SPKMenu;

class SPKMenuItem {
public:
    enum itemType { changesToMenu, sendsCommand, hasHandler };
    itemType type;
    string text;
    void (*handler)(int, bool);
    union {
        SPKMenu* menu;
        int32_t command[2];
        void (*handler)(int, bool);
    } payload;
    
    SPKMenuItem(string title, SPKMenu* menu)
    {
        text = title;
        type = changesToMenu;
        payload.menu = menu;
    }
    
    SPKMenuItem(void (*menuItemHandler)(int, bool))
    {
        text = "[has handler]";
        type = hasHandler;
        payload.handler = menuItemHandler;
    }
    
    SPKMenuItem(string title, int32_t command)
    {
        text = title;
        type = sendsCommand;
        payload.command[0] = command;
        payload.command[1] = 0;
    }
    
    SPKMenuItem(string title, int32_t command1, int32_t command2)
    {
        text = title;
        type = sendsCommand;
        payload.command[0] = command1;
        payload.command[1] = command2;
    }
};


class SPKMenu {
public:
    SPKMenu() {
        selected.set(0, 0, 0, true);
    }
    
    std::string title;
    
    SPKMenu& operator = (const int &newIndex) {
        selected = newIndex;
        return *this;
    }
    
    void operator ++ () {
        selected++;
    }
    
    void operator -- () {
        selected--;
    }
    
    void addMenuItem (SPKMenuItem menuItem) {
        items.push_back(menuItem);
        selected.setMax(items.size()-1);
    }
    
    void clearMenuItems() {
        items.clear();
        selected.setMax(0);
    }
    
    int selectedIndex() {
        return selected.index();
    }
    
    std::string  selectedString() {
    if (items.size() == 0) printf("SPKMenu no items");        
        return items[selected.index()].text;
    }
    
    SPKMenuItem selectedItem() {
    if (items.size() == 0) printf("SPKMenu no items");
        return items[selected.index()];
    }
        
protected:
    SPKIndexInRange selected;
    std::vector<SPKMenuItem> items;
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

class SPKMessageHold {
public:

    SPKMessageHold() {
        state = notHold;
        currentMessage = "";
        savedMessage = "";
    }
    
    void addMessage(string message)
    {
        addMessage(message, 0, 0);
    }
    
    void addMessage(string message, float maxSecs)
    {
        addMessage(message, 0.1, maxSecs);
    }
    
    void addMessage(string message, float minSecs, float maxSecs) 
    {
        if (state == notHold)
        {
            if (maxSecs > 0.0f)
            {
                state = (minSecs > 0) ? holdWaitingForMin : holdMinPassed;
                savedMessage = currentMessage;
                currentMessage = message;
                
                maxTimeout.attach(this, &SPKMessageHold::handleTimeout, maxSecs);
                if (minSecs > 0) minTimeout.attach(this, &SPKMessageHold::handleTimeout, minSecs);
            }
            else
            {
                currentMessage = message;
            }
        }
        if (state == holdWaitingForMin)
        {
            if (maxSecs > 0.0f)  enqueueMessage(message, minSecs, maxSecs);
            else                 savedMessage = message;
        }
        if (state == holdMinPassed)
        {
            if (maxSecs > 0.0f) { enqueueMessage(message, minSecs, maxSecs); dequeueMessage(); }
            else                savedMessage = message;
        }
    }
    
    string message() { return currentMessage; }

private:
    enum stateType { notHold, holdWaitingForMin, holdMinPassed };
    struct messageType { string message; float minSecs; float maxSecs; };

    void enqueueMessage(string message, float minSecs, float maxSecs)
    {
        messageType messageStruct = {message, minSecs, maxSecs};
        enqueuedMessages.push(messageStruct);
    }

    void dequeueMessage()
    {
        currentMessage = enqueuedMessages.front().message;
        float minSecs = enqueuedMessages.front().minSecs;
        float maxSecs = enqueuedMessages.front().maxSecs;
        enqueuedMessages.pop();
        
        maxTimeout.detach();
        minTimeout.detach();
        maxTimeout.attach(this, &SPKMessageHold::handleTimeout, maxSecs);
        if (minSecs > 0) minTimeout.attach(this, &SPKMessageHold::handleTimeout, minSecs);
        state = (minSecs > 0) ? holdWaitingForMin : holdMinPassed;
    }
    
    void handleTimeout() 
    {
        if (enqueuedMessages.empty())
        {
            if (state == holdWaitingForMin)
            {
                state = holdMinPassed;
            }
            else
            {
                currentMessage = savedMessage;
                state = notHold;
            }
        }
        else
        {
            dequeueMessage();
        }
    }
    
    stateType state;
    string currentMessage;
    string savedMessage;
    queue< messageType >enqueuedMessages;
    Timeout minTimeout;
    Timeout maxTimeout;
};