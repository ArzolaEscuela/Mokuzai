#include "Options.h"
#include<windows.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <CameraHandler.h>
#include <GL/freeglut.h>
#include <fstream>
#include <SOIL/SOIL.h>

#include <Shapes.h>
#include <Utilities.h>
#include <Images.h>
#include <Random.h>
#include <tuple>
#include <functional>

using namespace std;

const int margin = 10;
const int tabSize = 40;
const int spacing = 5;
const int MENU_WIDTH = 200;
static int selectedTab = 11;
const int INITIAL_X_OFFSET = tabSize + margin;
static vector<float> MENU_WHITE = { 1,1,1 };
static char* desc1 = " ";
static char* desc2 = " ";
static char* desc3 = " ";

static char* popupTitle = " ";
static char* popupSubTitle = " ";
static char* popupDesc = " ";
static char* popupSubDesc = " ";
static char* popupSubSubDesc = " ";
static char* errorText = " ";

// Text
static void* helv10 = (void*)GLUT_BITMAP_HELVETICA_10;
static void* helv12 = (void*)GLUT_BITMAP_HELVETICA_12;
static void* helv18 = (void*)GLUT_BITMAP_HELVETICA_18;
static void* time10 = (void*)GLUT_BITMAP_TIMES_ROMAN_10;
static void* time24 = (void*)GLUT_BITMAP_TIMES_ROMAN_24;
static void* eight = (void*)GLUT_BITMAP_8_BY_13;
static void* nine = (void*)GLUT_BITMAP_9_BY_15;

class Input;
static Input* infoDisplayer;
static Input* inputBeingEdited;

enum EEdit { ListeningFloat, ListeningInt, Error, NoMessage };
static EEdit editStatus = NoMessage;
static bool usedDot;
static bool usedMinus;
static vector<unsigned char> editInput;
enum EAction { DoNothing, ChangeSession, ChangeTexture, SaveModel, ToggleSave, ChangeTab, ToggleSubMenu, ToggleWireframe };

static void SaveSession();

template <typename T> string tostr(const T& t) {
   ostringstream os;
   os<<t;
   return os.str();
}

enum Anchor { BottomRight = 0, BottomLeft = 1, TopRight = 2, TopLeft = 3, TopCenter = 4, BottomCenter = 5 };
class Drawable
{
    public:
        Drawable() { };
    public:
        int id;

        bool selected;
        bool visible;

        float xStartClick;
        float xEndClick;
        float yStartClick;
        float yEndClick;

        EAction action;

        GLuint Texture() { return selected ? selectedTexture : normalTexture; }
        void RecalculateClickAreas(int w, int h)
        {
            xStartClick = (float(w) * anchorX) + offsetX;
            xEndClick = xStartClick + sizeX;
            yStartClick  = (float(h) * (1.0f - anchorY)) - offsetY;
            yEndClick = yStartClick + sizeY;

            switch(anchor)
            {
                case BottomLeft:
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                    break;
                case BottomRight:
                    xStartClick -= sizeX;
                    xEndClick -= sizeX;
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                    break;
                case TopLeft:
                    break;
                case TopRight:
                    xStartClick -= sizeX;
                    xEndClick -= sizeX;
                    break;
                case BottomCenter:
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                case TopCenter:
                    xStartClick -= float(sizeX) / 2.0;
                    break;
            }
        }
        virtual bool IsClickValid(int clickedX, int clickedY)
        {
            if (!visible) { return false; }

            if (clickedX < xStartClick) { return false; }
            if (clickedX > xEndClick) { return false; }
            if (clickedY < yStartClick) { return false; }
            if (clickedY > yEndClick) { return false; }

            return true;
        }
        virtual void DrawSelf()
        {
            if (!visible) { return; }

            Images::LoadTextureStart(Texture());
            glBegin (GL_QUADS);
                glTexCoord2f(0.02,0.02);
                glVertex2d(xStartClick,yStartClick);
                glTexCoord2f(0.02,0.98);
                glVertex2d(xStartClick,yEndClick);
                glTexCoord2f(0.98,0.98);
                glVertex2d(xEndClick,yEndClick);
                glTexCoord2f(0.98,0.02);
                glVertex2d(xEndClick,yStartClick);
            glEnd();
            Images::LoadTextureEnd();
        }
    protected:
        Anchor anchor;
        float anchorX;
        float anchorY;

        int sizeX = 10;
        int sizeY = 10;

        int offsetX = 0;
        int offsetY = 0;

        GLuint normalTexture;
        GLuint selectedTexture;
};

class MenuItem : public Drawable
{
    public:
        MenuItem(float x, float y, int offX, int offY, int xSize, int ySize, Anchor aType, EAction type, GLuint tex, int identifier = -1)
        {
            anchorX = x;
            anchorY = y;
            sizeX = xSize;
            sizeY = ySize;
            offsetX = offX;
            offsetY = offY;

            anchor = aType;
            action = type;
            normalTexture = tex;
            id = identifier;

            selected = false;
            visible = true;

            RecalculateClickAreas(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        }
        MenuItem(){}
    public:
        void RecalculateClickAreas(int w, int h)
        {
            float relativeXStart = float(INITIAL_X_OFFSET) / float(w);
            float relativeXEnd = float(INITIAL_X_OFFSET + MENU_WIDTH) / float(w);
            float realAnchorX = relativeXStart + ((relativeXEnd - relativeXStart) * anchorX);

            xStartClick = (float(w) * realAnchorX) + offsetX;
            xEndClick = xStartClick + sizeX;
            yStartClick  = (float(h) * (1.0f - anchorY)) - offsetY;
            yEndClick = yStartClick + sizeY;

            switch(anchor)
            {
                case BottomLeft:
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                    break;
                case BottomRight:
                    xStartClick -= sizeX;
                    xEndClick -= sizeX;
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                    break;
                case TopLeft:
                    break;
                case TopRight:
                    xStartClick -= sizeX;
                    xEndClick -= sizeX;
                    break;
                case BottomCenter:
                    yStartClick -= sizeY;
                    yEndClick -= sizeY;
                case TopCenter:
                    xStartClick -= float(sizeX) / 2.0;
                    break;
            }
        }
};

class Button : public Drawable
{
    public:
        Button(float x, float y, int offX, int offY, int xSize, int ySize, Anchor aType, EAction type, GLuint tex, GLuint tex2, int identifier = -1)
        {
            anchorX = x;
            anchorY = y;
            sizeX = xSize;
            sizeY = ySize;
            offsetX = offX;
            offsetY = offY;

            anchor = aType;
            action = type;
            normalTexture = tex;
            selectedTexture = tex2;
            id = identifier;

            selected = false;
            visible = true;

            RecalculateClickAreas(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        }
        Button(){}
};

class ToggleButton : public Button
{
    public:
        ToggleButton(float x, float y, int offX, int offY, int xSize, int ySize, Anchor aType, EAction type, GLuint tex, GLuint tex2, vector<Button*> toggles, int identifier = -1)
        {
            anchorX = x;
            anchorY = y;
            sizeX = xSize;
            sizeY = ySize;
            offsetX = offX;
            offsetY = offY;

            anchor = aType;
            action = type;
            normalTexture = tex;
            selectedTexture = tex2;
            id = identifier;

            visible = true;

            toggeableButtons = toggles;

            RecalculateClickAreas(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

            selected = true;
            ToggleVisibility();
        }
        ToggleButton(){}
    public:
        vector<Button*> toggeableButtons;
        void ToggleVisibility()
        {
            int buttonCount = toggeableButtons.size();
            for (int i = 0; i < buttonCount; i++)
            {
                toggeableButtons[i]->visible = !toggeableButtons[i]->visible;
            }
            selected = !selected;
        }
        bool IsClickValid(int clickedX, int clickedY)
        {
            if (Drawable::IsClickValid(clickedX, clickedY))
            {
                ToggleVisibility();
            }
            return false;
        }
};

class Tab : public Drawable
{
    public:
        Tab(float x, float y, int offX, int offY, int xSize, int ySize, Anchor aType, EAction type, GLuint tex, GLuint tex2, int identifier = -1)
        {
            anchorX = x;
            anchorY = y;
            sizeX = xSize;
            sizeY = ySize;
            offsetX = offX;
            offsetY = offY;

            anchor = aType;
            action = type;
            normalTexture = tex;
            selectedTexture = tex2;
            id = identifier;

            selected = false;
            visible = true;

            RecalculateClickAreas(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        }
        Tab(float x, float y, int xSize, int ySize, EAction type, GLuint tex, int identifier = -1)
        {
            anchorX = x;
            anchorY = y;
            sizeX = xSize;
            sizeY = ySize;

            action = type;
            normalTexture = tex;
            id = identifier;

            selected = false;
            visible = true;

            RecalculateClickAreas(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        }
        Tab(){}
    public:
};

class TextEntry : public MenuItem
{
    public:
        TextEntry(float x, float y, int offX, int offY, char* text, Anchor a, void* fontToUse)
        {
            offsetX = offX;
            offsetY = offY;
            anchorX = x;
            anchorY = y;
            textToDraw = text;
            font = fontToUse;

            action = DoNothing;
            anchor = a;

        }
        TextEntry(){}
    public:
        char* textToDraw;
        void* font;
        void DrawSelf()
        {
            glPushMatrix();
                glLoadIdentity();
                glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]);

                char *c;
                int fontHeight = glutBitmapHeight(font);
                int h = glutGet(GLUT_WINDOW_HEIGHT);

                float x = INITIAL_X_OFFSET + (MENU_WIDTH * anchorX) + offsetX;
                float y = ((1.0f - anchorY) * h) - offsetY;

                glRasterPos2f(x, y);
                for (c=textToDraw; *c != '\0'; c++) { glutBitmapCharacter(font, *c); }
            glPopMatrix();
        }
};

class Input : public MenuItem
{
    public:
        virtual void ToggleInfo() {}
        virtual void BeginEdit() {}
        virtual void CancelEdit() {}
        virtual void AttemptToAcceptInput(){}
        virtual void EditText(){}
};

static GLint selectedInfoImage;
static GLint deselectedInfoImage;
static GLint selectedEditImage;
static GLint deselectedEditImage;
const int INPUT_BUTTON_SIZE = 14;
const int INPUT_LINE_DISTANCE = 16;
const int INPUT_HEIGHT = margin + 7 + INPUT_LINE_DISTANCE;
template <typename T>
class InputValue : public Input
{
    public:
        InputValue(int offset, T minVal, T maxVal, char* title, char* desc, function<T()> getter, function<void(T)> setter)
        {
            GetValue = getter;
            ReallySetValue = setter;

            offsetY = offset;
            minValue = minVal;
            maxValue = maxVal;
            selected = false;
            visible = true;

            name = title;
            description = desc;

            ostringstream oss;
            oss.clear();
            oss << "This value needs to be between " << minValue << " and " << maxValue << "." << endl;
            limits = oss.str();
            oss.clear();

            ostringstream oss2;
            oss2.clear();
            oss2 << "Type a new value for: \"" << name << "\"." << endl;
            typeAValue = oss2.str();
            oss2.clear();

            if (selectedInfoImage == NULL) { selectedInfoImage = Images::GetImageTextureFromFilePath("UI\\Info_Selected.png"); }
            if (deselectedInfoImage == NULL) { deselectedInfoImage = Images::GetImageTextureFromFilePath("UI\\Info.png"); }
            if (selectedEditImage == NULL) { selectedEditImage = Images::GetImageTextureFromFilePath("UI\\Edit_Selected.png"); }
            if (deselectedEditImage == NULL) { deselectedEditImage = Images::GetImageTextureFromFilePath("UI\\Edit.png"); }
        }
        InputValue(){}
    public:
        T minValue;
        T maxValue;
        void ToggleInfo()
        {
            selected = !selected;

            if (!selected)
            {
                desc1 = "";
                desc2 = "";
                desc3 = "";
                infoDisplayer = NULL;
                return;
            }

            if (infoDisplayer != NULL) { infoDisplayer->ToggleInfo(); }

            desc1 = name;
            desc2 = description;
            desc3 = const_cast<char*>(limits.c_str());;
            infoDisplayer = this;
        }
        void DrawSelf()
        {
            // Draw Text
            glPushMatrix();
                glLoadIdentity();
                glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]);
                char *c;
                glRasterPos2f(LeftX(), FirstLineStart());
                for (c=name; *c != '\0'; c++) { glutBitmapCharacter(nine, *c); }
                glRasterPos2f(LeftX()+45, SecondLineStart());
                for (c=Utilities::Concatenate("Current Value: ", GetValue()); *c != '\0'; c++) { glutBitmapCharacter(helv12, *c); }
            glPopMatrix();

            float buttonBottomY = ButtonStartY();
            float buttonTopY = ButtonEndY();
            // Draw Buttons
            Images::LoadTextureStart(InfoTexture());
            glBegin (GL_QUADS);
                glTexCoord2f(0.02,0.02);
                glVertex2d(InfoButtonStartX(),buttonBottomY);
                glTexCoord2f(0.02,0.98);
                glVertex2d(InfoButtonStartX(), buttonTopY);
                glTexCoord2f(0.98,0.98);
                glVertex2d(InfoButtonEndX(),buttonTopY);
                glTexCoord2f(0.98,0.02);
                glVertex2d(InfoButtonEndX(),buttonBottomY);
            glEnd();
            Images::LoadTextureEnd();

            Images::LoadTextureStart(EditTexture());
            glBegin (GL_QUADS);
                glTexCoord2f(0.02,0.02);
                glVertex2d(EditButtonStartX(),buttonBottomY);
                glTexCoord2f(0.02,0.98);
                glVertex2d(EditButtonStartX(), buttonTopY);
                glTexCoord2f(0.98,0.98);
                glVertex2d(EditButtonEndX(), buttonTopY);
                glTexCoord2f(0.98,0.02);
                glVertex2d(EditButtonEndX(), buttonBottomY);
            glEnd();
            Images::LoadTextureEnd();
        }
        bool IsClickValid(int clickedX, int clickedY)
        {
            if (!visible) { return false; }

            if (clickedX > InfoButtonStartX() && clickedX < InfoButtonEndX()
                && clickedY > ButtonStartY() && clickedY < ButtonEndY())
            {
                // Info Clicked
                ToggleInfo();
                return false;
            }

            if (clickedX > EditButtonStartX() && clickedX < EditButtonEndX()
                && clickedY > ButtonStartY() && clickedY < ButtonEndY())
            {
               if (inputBeingEdited != NULL) { inputBeingEdited->CancelEdit(); }
               editInput.clear();

               usedDot = false;
               usedMinus = false;
               editInput.clear();
               inputBeingEdited = this;
               editStatus = std::is_same<T, int>::value ? ListeningInt : ListeningFloat;

                popupTitle = "Listening For Input";
                popupSubTitle = const_cast<char*>(typeAValue.c_str());
                popupDesc = "(Press ESC To Cancel / ENTER To Accept)";
                errorText = const_cast<char*>(limits.c_str());
            }
            return false;
        }
        void EditText()
        {
            popupSubDesc = const_cast<char*>(limits.c_str());
            string s;
            for (unsigned char c: editInput) { s.push_back(c); }
            popupSubSubDesc = const_cast<char*>(s.c_str());
            if (editInput.size() == 0) { popupSubSubDesc = "0"; }
        }
        void CancelEdit()
        {
            editStatus = NoMessage;
            usedDot = false;
            usedMinus = false;
            inputBeingEdited = NULL;
            editInput.clear();
        }
        void AttemptToAcceptInput()
        {
            string s;
            for (unsigned char c: editInput) { s.push_back(c); }
            popupSubSubDesc = const_cast<char*>(s.c_str());

            if (std::is_same<T, int>::value)
            {
                // PROCESS INT
                int inputInt = atoi(s.c_str());
                if (inputInt > maxValue || inputInt < minValue)
                {
                    editStatus = Error;
                    return;
                }
                ReallySetValue(inputInt);
            }
            else
            {
                // PROCESS FLOAT
                float inputFloat = strtof(s.c_str(), NULL);
                if (inputFloat > maxValue || inputFloat < minValue)
                {
                    editStatus = Error;
                    return;
                }
                ReallySetValue(inputFloat);
            }

            SaveSession();
            CancelEdit();
        }
    private:
        float LeftX() { return INITIAL_X_OFFSET + margin; }
        float FirstLineStart(){ return offsetY + INPUT_HEIGHT - INPUT_LINE_DISTANCE; }
        float SecondLineStart(){ return FirstLineStart() + INPUT_LINE_DISTANCE; }

        float ButtonStartY() { return SecondLineStart() - (INPUT_BUTTON_SIZE/2) - 5; }
        float ButtonEndY() { return ButtonStartY() + INPUT_BUTTON_SIZE; }
        float InfoButtonStartX() { return LeftX(); }
        float InfoButtonEndX() { return InfoButtonStartX() + INPUT_BUTTON_SIZE; }
        float EditButtonStartX() { return InfoButtonEndX() + spacing; }
        float EditButtonEndX() { return EditButtonStartX() + INPUT_BUTTON_SIZE; }

        GLint EditTexture()
        {
            if (inputBeingEdited == this) { return selectedEditImage; }
            return deselectedEditImage;
        }
        GLint InfoTexture()
        {
            if (infoDisplayer == this) { return selectedInfoImage; }
            return deselectedInfoImage;
        }
        bool selectedEdit;
        char* name;
        char* description;
        string limits;
        string typeAValue;
        function<T()> GetValue;
        function<void(T)> ReallySetValue;
};

class Menu
{
    public:
        Menu(int tab, vector<MenuItem*> items)
        {
            tabID = tab;
            menuItems = items;
        }
        Menu(){}
    public:
        vector<MenuItem*> menuItems;
        int tabID;
        void OnTabChange(int newTabID)
        {
            int itemCount = menuItems.size();
            for (int i = 0; i < itemCount; i++) { menuItems[i]->visible = newTabID == tabID; }
        }
        void RecalculateClickAreas(int w, int h)
        {
            int itemCount = menuItems.size();
            for (int i = 0; i < itemCount; i++) { menuItems[i]->RecalculateClickAreas(w,h); }
        }
        void DrawMenu()
        {
            if (selectedTab != tabID) { return; }
            int itemCount = menuItems.size();
            for (int i = 0; i < itemCount; i++) { menuItems[i]->DrawSelf(); }
        }
        void AttemptToClick(int x, int y)
        {
            int itemCount = menuItems.size();
            for (int i = 0; i < itemCount; i++) { menuItems[i]->IsClickValid(x,y); }
        }
};

struct BranchLevel
{
    public:
        float branchLength = 0.6;
        float branchLengthDelta = 0.1;
        float branchBase = 0.01;
        float branchBaseDelta = 0;
        float branchTip = 0.075;
        float branchTipDelta = 0;
        float branchSplitAngle = 20;
        float branchSplitAngleDelta = 10;
        float branchCurvature = 90;
        float branchCurvatureDelta = 10;
        float branchFirstCurve = 0;
        float branchFirstCurveDelta = 0;
        float branchSecondCurve = 0;
        float branchSecondCurveDelta = 0;
        float initialUnusedSpace = 0.3;
        float initialUnusedSpaceDelta = 0.1;
        float branchSeparation = 75;
        float branchSeparationDelta = 5;
        int branchSubdivisions = 5;
        int branchCount = 6;
        int branchCountDelta = 2;
};

//------------------------------------------------------------------------------------//
/*----------------------------------- FIELDS -----------------------------------------*/
//------------------------------------------------------------------------------------//

// UI
ToggleButton* textureButton;
ToggleButton* saveButton;
ToggleButton* sessionButton;
Button* wireframeButton;

static vector<Menu*> menus;
static vector<Drawable*> allDrawables;

static vector<Tab*> tabs;
static vector<Button*> sessionButtons;
static vector<Button*> saveButtons;
static vector<Button*> treeTextureButtons;
static vector<Button*> leavesTextureButtons;
static vector<Button*> groundTextureButtons;

// Persistence
const char* optionsFileName = "Options";
const char* sessionFileName = "Session";
const int MAX_SESSIONS = 3;

// Options Info
static int selectedSession = 1;
static bool renderWireframe = false;

// Session Info
static int seed = 3000;
static int branchQuality = 8;
const int MAX_PREVIEW_TEXTURES = 3;
static int treeTexture = 0;
static int leavesTexture = 0;
static int groundTexture = 0;
static int maxLevels = 2;

static int leafType = 4;
static int leafCount = 25;
static int leafCountDelta = 3;
static float leafSize = 0.5;
static float leafSizeDelta = 0.02;
static float leafSizeX = 0.75;
static float leafSizeXDelta = 0.1;
static float leafBend = 0.1;
static float leafBendDelta = 0.1;
static int leafZoom = 5;
static int minLeafLevel = 0;
static float leafOffset = 0.05;
static float leafOffsetDelta = 0.05;

static BranchLevel stump;
static BranchLevel level1;
static BranchLevel level2;
static BranchLevel level3;

//------------------------------------------------------------------------------------//
/*---------------------------------- METHODS -----------------------------------------*/
//------------------------------------------------------------------------------------//

int Options::GetTreeTexture() { return treeTexture; }
int Options::GetLeavesTexture() { return leavesTexture; }
int Options::GetGroundTexture() { return groundTexture; }
int Options::GetSession() { return selectedSession; }


///--------------------------///
///--------- GENERAL --------///
///--------------------------///
int Options::GetMaxLevels() { return maxLevels; } // Base 0, 0 = Only Stump
int Options::GetBranchQuality() { return branchQuality; }
bool Options::GetRenderWireframe() { return renderWireframe || CameraHandler::IsMoving(); }

///--------------------------///
///--------- LEAVES --------///
///--------------------------///
int Options::GetLeafType() { return leafType; }
int Options::GetMinLeafLevel() { return minLeafLevel; }
int Options::GetLeafCount() { return Random::NextRandomDeltaIntClamped(1,200, leafCount,leafCountDelta); }
int Options::GetLeafZoom() { return leafZoom; }
float Options::GetLeafSize() { return Random::NextRandomDeltaClamped(leafSize, leafSizeDelta, 0.01, 4); }
float Options::GetLeafSizeX() { return Random::NextRandomDeltaClamped(leafSizeX, leafSizeXDelta, 0.01, 4); }
float Options::GetLeafBend() { return Random::NextRandomDeltaClamped(leafBend, leafBendDelta, -90, 90); }
float Options::GetLeafOffset() { return Random::NextRandomDeltaClamped(leafOffset, leafOffsetDelta, 0, 3); }

///--------------------------///
///-------- BRANCHES --------///
///--------------------------///

///-------- SIZE --------///
float Options::GetBranchLength(int level) // Random Between -n - m
{
    if (level == 0) { return Random::NextRandomDelta(stump.branchLength, stump.branchLengthDelta); }
    if (level == 1) { return Random::NextRandomDelta(level1.branchLength, level1.branchLengthDelta) * GetBranchLength(level-1); }
    if (level == 2) { return Random::NextRandomDelta(level2.branchLength, level2.branchLengthDelta) * GetBranchLength(level-1); }
    if (level == 3) { return Random::NextRandomDelta(level3.branchLength, level3.branchLengthDelta) * GetBranchLength(level-1); }
    throw "Invalid Level";
}
float Options::GetBranchBaseRadius() // Random between 0.1-n
{
    return Random::NextRandomDelta(stump.branchBase, stump.branchBaseDelta);
}
float Options::GetBranchTipRadius(int level) // Random Between 0-1
{
    if (level == 0) { return Random::NextRandomDeltaClamped(stump.branchTip, stump.branchTipDelta, 0, 1); }
    if (level == 1) { return Random::NextRandomDeltaClamped(level1.branchTip, level1.branchTipDelta, 0, 1); }
    if (level == 2) { return Random::NextRandomDeltaClamped(level2.branchTip, level2.branchTipDelta, 0, 1); }
    if (level == 3) { return Random::NextRandomDeltaClamped(level3.branchTip, level3.branchTipDelta, 0, 1); }
    throw "Invalid Level";
}
///-------- BRANCHING --------///
float Options::GetInitialUnusedSpace(int level) // Random Between 0-1
{
    if (level == 0) { return Random::NextRandomDeltaClamped(stump.initialUnusedSpace, stump.initialUnusedSpaceDelta, 0, 0.95); }
    if (level == 1) { return Random::NextRandomDeltaClamped(level1.initialUnusedSpace, level1.initialUnusedSpaceDelta, 0, 0.95); }
    if (level == 2) { return Random::NextRandomDeltaClamped(level2.initialUnusedSpace, level2.initialUnusedSpaceDelta, 0, 0.95); }
    if (level == 3) { return Random::NextRandomDeltaClamped(level3.initialUnusedSpace, level3.initialUnusedSpaceDelta, 0, 0.95); }
    return 0;
}
int Options::GetBranchCount(int level) // Random Between 1-n
{
    if (level == 0) { return Random::NextRandomDeltaIntClamped(0,200,stump.branchCount, stump.branchCountDelta); }
    if (level == 1) { return Random::NextRandomDeltaIntClamped(0,200,level1.branchCount, level1.branchCountDelta); }
    if (level == 2) { return Random::NextRandomDeltaIntClamped(0,200,level2.branchCount, level2.branchCountDelta); }
    if (level == 3) { return Random::NextRandomDeltaIntClamped(0,200, level3.branchCount, level3.branchCountDelta); }
    throw "Invalid Level";
}
int Options::GetBranchSubdivisions(int level)
{
    if (level == 0) { return stump.branchSubdivisions;  }
    if (level == 1) { return level1.branchSubdivisions; }
    if (level == 2) { return level2.branchSubdivisions; }
    if (level == 3) { return level3.branchSubdivisions; }
    throw "Invalid Level";
}
///-------- CURVATURE --------///
float Options::GetBranchSplitAngle(int level) // Random Between 0-180
{
    if (level == 0) { return Random::NextRandomDeltaClamped(stump.branchSplitAngle, stump.branchSplitAngleDelta, -180, 180); }
    if (level == 1) { return Random::NextRandomDeltaClamped(level1.branchSplitAngle, level1.branchSplitAngleDelta, -180, 180); }
    if (level == 2) { return Random::NextRandomDeltaClamped(level2.branchSplitAngle, level2.branchSplitAngleDelta, -180, 180); }
    if (level == 3) { return Random::NextRandomDeltaClamped(level3.branchSplitAngle, level3.branchSplitAngleDelta, -180, 180); }
    throw "Invalid Level";
}
float Options::GetBranchCurvature(int level) // Between 0 and 180;
{
    if (level == 0) { return Random::NextRandomDeltaClamped(stump.branchCurvature, stump.branchCurvatureDelta, -180, 180); }
    if (level == 1) { return Random::NextRandomDeltaClamped(level1.branchCurvature, level1.branchCurvatureDelta, -180, 180); }
    if (level == 2) { return Random::NextRandomDeltaClamped(level2.branchCurvature, level2.branchCurvatureDelta, -180, 180); }
    if (level == 3) { return Random::NextRandomDeltaClamped(level3.branchCurvature, level3.branchCurvatureDelta, -180, 180); }
    throw "Invalid Level";
}
float Options::GetBranchSeparation(int level)
{
    if (level == 0) { return Random::NextRandomDeltaClamped(stump.branchSeparation, stump.branchSeparationDelta, 0, 360); }
    if (level == 1) { return Random::NextRandomDeltaClamped(level1.branchSeparation, level1.branchSeparationDelta, 0, 360); }
    if (level == 2) { return Random::NextRandomDeltaClamped(level2.branchSeparation, level2.branchSeparationDelta, 0, 360); }
    if (level == 3) { return Random::NextRandomDeltaClamped(level3.branchSeparation, level3.branchSeparationDelta, 0, 360); }
    throw "Invalid Level";
}
/// TODO
float Options::GetSubdivisionFirstHalfRotation(int level) // Random Between 0-180
{
    if (level == 0) { return Random::NextRandomDelta(stump.branchFirstCurve, stump.branchFirstCurveDelta); }
    if (level == 1) { return Random::NextRandomDelta(level1.branchFirstCurve, level1.branchFirstCurveDelta); }
    if (level == 2) { return Random::NextRandomDelta(level2.branchFirstCurve, level2.branchFirstCurveDelta); }
    if (level == 3) { return Random::NextRandomDelta(level3.branchFirstCurve, level3.branchFirstCurveDelta); }
    throw "Invalid Level";
}
/// TODO
float Options::GetSubdivisionSecondHalfRotation(int level) // Random Between 0-180
{
    if (level == 0) { return Random::NextRandomDelta(stump.branchSecondCurve, stump.branchSecondCurveDelta); }
    if (level == 1) { return Random::NextRandomDelta(level1.branchSecondCurve, level1.branchSecondCurveDelta); }
    if (level == 2) { return Random::NextRandomDelta(level2.branchSecondCurve, level2.branchSecondCurveDelta); }
    if (level == 3) { return Random::NextRandomDelta(level3.branchSecondCurve, level3.branchSecondCurveDelta); }
    throw "Invalid Level";
}

static void SaveOptions()
{
    // Save On Options File
    ofstream file;
    file.open(Utilities::FilePath(optionsFileName));
    if (!file.is_open()) { printf("ERROR: Unable to open file; \"%s\"\n", optionsFileName); return; }

    // Line #1: Selected Session
    file << selectedSession << endl;
    file << (renderWireframe ? 1 : 0) << endl;

    file.close();
    printf("Options Updated\n");
}

static void LoadOptions()
{
    // Load Options File
    ifstream file;
    file.open(Utilities::FilePath(optionsFileName));
    if (!file.is_open())
    {
        // The File Doesn't Exist
        SaveOptions();
        LoadOptions();
        return;
    }

    int currentLine = 0;
    for(string line; getline( file, line ); )
    {
        currentLine++;
        switch(currentLine)
        {
            case 1: // Line #1: Selected Session
                selectedSession = atoi(line.c_str());
                continue;
            case 2: // Line #2 : Wireframe
                renderWireframe = atoi(line.c_str()) == 1;
                continue;
        }
    }

    file.close();
    printf("Options Successfully Loaded.\n");
}

static void SaveSession()
{
    // Save On Session File
    char* selectedSessionFileName = Utilities::FilePath(Utilities::Concatenate(sessionFileName, selectedSession));

    ofstream file;
    file.open(selectedSessionFileName);
    if (!file.is_open()) { printf("ERROR: Unable to open file; \"%s\"\n", selectedSessionFileName); return; }

    // Line #1: Seed
    file << seed << endl;

    // Line #2-4: Preview Textures
    file << treeTexture << endl;
    file << leavesTexture << endl;
    file << groundTexture << endl;

    // Line #5: Max Tree Levels
    file << maxLevels << endl;

    // Line #6-24: Stump
    file << stump.branchLength << endl;
    file << stump.branchLengthDelta << endl;
    file << stump.branchBase << endl;
    file << stump.branchBaseDelta << endl;
    file << stump.branchTip << endl;
    file << stump.branchTipDelta << endl;
    file << stump.branchSplitAngle << endl;
    file << stump.branchSplitAngleDelta << endl;
    file << stump.branchCurvature << endl;
    file << stump.branchCurvatureDelta << endl;
    file << stump.branchFirstCurve << endl;
    file << stump.branchFirstCurveDelta << endl;
    file << stump.branchSecondCurve << endl;
    file << stump.branchSecondCurveDelta << endl;
    file << stump.initialUnusedSpace << endl;
    file << stump.initialUnusedSpaceDelta << endl;
    file << stump.branchSubdivisions << endl;
    file << stump.branchCount << endl;
    file << stump.branchCountDelta << endl;

    // Line #25-43: Level 1
    file << level1.branchLength << endl;
    file << level1.branchLengthDelta << endl;
    file << level1.branchBase << endl;
    file << level1.branchBaseDelta << endl;
    file << level1.branchTip << endl;
    file << level1.branchTipDelta << endl;
    file << level1.branchSplitAngle << endl;
    file << level1.branchSplitAngleDelta << endl;
    file << level1.branchCurvature << endl;
    file << level1.branchCurvatureDelta << endl;
    file << level1.branchFirstCurve << endl;
    file << level1.branchFirstCurveDelta << endl;
    file << level1.branchSecondCurve << endl;
    file << level1.branchSecondCurveDelta << endl;
    file << level1.initialUnusedSpace << endl;
    file << level1.initialUnusedSpaceDelta << endl;
    file << level1.branchSubdivisions << endl;
    file << level1.branchCount << endl;
    file << level1.branchCountDelta << endl;

    // Line #44-62: Level 2
    file << level2.branchLength << endl;
    file << level2.branchLengthDelta << endl;
    file << level2.branchBase << endl;
    file << level2.branchBaseDelta << endl;
    file << level2.branchTip << endl;
    file << level2.branchTipDelta << endl;
    file << level2.branchSplitAngle << endl;
    file << level2.branchSplitAngleDelta << endl;
    file << level2.branchCurvature << endl;
    file << level2.branchCurvatureDelta << endl;
    file << level2.branchFirstCurve << endl;
    file << level2.branchFirstCurveDelta << endl;
    file << level2.branchSecondCurve << endl;
    file << level2.branchSecondCurveDelta << endl;
    file << level2.initialUnusedSpace << endl;
    file << level2.initialUnusedSpaceDelta << endl;
    file << level2.branchSubdivisions << endl;
    file << level2.branchCount << endl;
    file << level2.branchCountDelta << endl;

    // Line #63-81: Level 3
    file << level3.branchLength << endl;
    file << level3.branchLengthDelta << endl;
    file << level3.branchBase << endl;
    file << level3.branchBaseDelta << endl;
    file << level3.branchTip << endl;
    file << level3.branchTipDelta << endl;
    file << level3.branchSplitAngle << endl;
    file << level3.branchSplitAngleDelta << endl;
    file << level3.branchCurvature << endl;
    file << level3.branchCurvatureDelta << endl;
    file << level3.branchFirstCurve << endl;
    file << level3.branchFirstCurveDelta << endl;
    file << level3.branchSecondCurve << endl;
    file << level3.branchSecondCurveDelta << endl;
    file << level3.initialUnusedSpace << endl;
    file << level3.initialUnusedSpaceDelta << endl;
    file << level3.branchSubdivisions << endl;
    file << level3.branchCount << endl;
    file << level3.branchCountDelta << endl;

    // Line #82-89 Branch Separation
    file << stump.branchSeparation << endl;
    file << stump.branchSeparationDelta << endl;
    file << level1.branchSeparation << endl;
    file << level1.branchSeparationDelta << endl;
    file << level2.branchSeparation << endl;
    file << level2.branchSeparationDelta << endl;
    file << level3.branchSeparation << endl;
    file << level3.branchSeparationDelta << endl;

    // Line #90 Branch Quality
    file << branchQuality << endl;

    // Line #91-103 Leaf Settings
    file << leafType << endl;
    file << leafCount << endl;
    file << leafCountDelta << endl;
    file << leafSize << endl;
    file << leafSizeDelta << endl;
    file << leafSizeX << endl;
    file << leafSizeXDelta << endl;
    file << leafBend << endl;
    file << leafBendDelta << endl;
    file << leafZoom << endl;
    file << minLeafLevel << endl;
    file << leafOffset << endl;
    file << leafOffsetDelta << endl;

    file.close();
    printf("Changes Saved For Session #%d\n", selectedSession);
}

static void HighlightSelections()
{
    treeTextureButtons[treeTexture]->selected = true;
    leavesTextureButtons[leavesTexture]->selected = true;
    groundTextureButtons[groundTexture]->selected = true;
    wireframeButton->selected = renderWireframe;

    int tabSize = tabs.size();
    for (int i = 0; i < tabSize; i++) { tabs[i]->selected = tabs[i]->id == selectedTab; }
}

static void LoadSession(bool startCheck = false)
{
    // Save On Session File
    char* selectedSessionFileName = Utilities::FilePath(Utilities::Concatenate(sessionFileName, selectedSession));

    ifstream file;
    file.open(selectedSessionFileName);
    if (!file.is_open())
    {
        // The File Doesn't Exist
        SaveSession();
        LoadSession();
        return;
    }

    int currentLine = 0;
    for(string line; getline( file, line ); )
    {
        currentLine++;
        switch(currentLine)
        {
            case 1: // Line #1: Seed
                seed = atoi(line.c_str());
                continue;
            case 2:  // Line #2-4: Preview Textures
                treeTexture = atoi(line.c_str());
                continue;
            case 3:  // Line #2-4: Preview Textures
                leavesTexture = atoi(line.c_str());
                continue;
            case 4:  // Line #2-4: Preview Textures
                groundTexture = atoi(line.c_str());
                continue;
            case 5: // Line #5: Max Tree Levels
                maxLevels = atoi(line.c_str());
                continue;
            // Line #6-24: Stump
            case 6:
                stump.branchLength = strtof(line.c_str(), NULL);
                continue;
            case 7:
                stump.branchLengthDelta = strtof(line.c_str(), NULL);
                continue;
            case 8:
                stump.branchBase = strtof(line.c_str(), NULL);
                continue;
            case 9:
                stump.branchBaseDelta = strtof(line.c_str(), NULL);
                continue;
            case 10:
                stump.branchTip = strtof(line.c_str(), NULL);
                continue;
            case 11:
                stump.branchTipDelta = strtof(line.c_str(), NULL);
                continue;
            case 12:
                stump.branchSplitAngle = strtof(line.c_str(), NULL);
                continue;
            case 13:
                stump.branchSplitAngleDelta = strtof(line.c_str(), NULL);
                continue;
            case 14:
                stump.branchCurvature = strtof(line.c_str(), NULL);
                continue;
            case 15:
                stump.branchCurvatureDelta = strtof(line.c_str(), NULL);
                continue;
            case 16:
                stump.branchFirstCurve = strtof(line.c_str(), NULL);
                continue;
            case 17:
                stump.branchFirstCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 18:
                stump.branchSecondCurve = strtof(line.c_str(), NULL);
                continue;
            case 19:
                stump.branchSecondCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 20:
                stump.initialUnusedSpace = strtof(line.c_str(), NULL);
                continue;
            case 21:
                stump.initialUnusedSpaceDelta = strtof(line.c_str(), NULL);
                continue;
            case 22:
                stump.branchSubdivisions = atoi(line.c_str());
                continue;
            case 23:
                stump.branchCount = atoi(line.c_str());
                continue;
            case 24:
                stump.branchCountDelta = atoi(line.c_str());
                continue;
            // Line #25-43: Level 1
            case 25:
                level1.branchLength = strtof(line.c_str(), NULL);
                continue;
            case 26:
                level1.branchLengthDelta = strtof(line.c_str(), NULL);
                continue;
            case 27:
                level1.branchBase = strtof(line.c_str(), NULL);
                continue;
            case 28:
                level1.branchBaseDelta = strtof(line.c_str(), NULL);
                continue;
            case 29:
                level1.branchTip = strtof(line.c_str(), NULL);
                continue;
            case 30:
                level1.branchTipDelta = strtof(line.c_str(), NULL);
                continue;
            case 31:
                level1.branchSplitAngle = strtof(line.c_str(), NULL);
                continue;
            case 32:
                level1.branchSplitAngleDelta = strtof(line.c_str(), NULL);
                continue;
            case 33:
                level1.branchCurvature = strtof(line.c_str(), NULL);
                continue;
            case 34:
                level1.branchCurvatureDelta = strtof(line.c_str(), NULL);
                continue;
            case 35:
                level1.branchFirstCurve = strtof(line.c_str(), NULL);
                continue;
            case 36:
                level1.branchFirstCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 37:
                level1.branchSecondCurve = strtof(line.c_str(), NULL);
                continue;
            case 38:
                level1.branchSecondCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 39:
                level1.initialUnusedSpace = strtof(line.c_str(), NULL);
                continue;
            case 40:
                level1.initialUnusedSpaceDelta = strtof(line.c_str(), NULL);
                continue;
            case 41:
                level1.branchSubdivisions = atoi(line.c_str());
                continue;
            case 42:
                level1.branchCount = atoi(line.c_str());
                continue;
            case 43:
                level1.branchCountDelta = atoi(line.c_str());
                continue;
            // Line #44-62: Level 2
            case 44:
                level2.branchLength = strtof(line.c_str(), NULL);
                continue;
            case 45:
                level2.branchLengthDelta = strtof(line.c_str(), NULL);
                continue;
            case 46:
                level2.branchBase = strtof(line.c_str(), NULL);
                continue;
            case 47:
                level2.branchBaseDelta = strtof(line.c_str(), NULL);
                continue;
            case 48:
                level2.branchTip = strtof(line.c_str(), NULL);
                continue;
            case 49:
                level2.branchTipDelta = strtof(line.c_str(), NULL);
                continue;
            case 50:
                level2.branchSplitAngle = strtof(line.c_str(), NULL);
                continue;
            case 51:
                level2.branchSplitAngleDelta = strtof(line.c_str(), NULL);
                continue;
            case 52:
                level2.branchCurvature = strtof(line.c_str(), NULL);
                continue;
            case 53:
                level2.branchCurvatureDelta = strtof(line.c_str(), NULL);
                continue;
            case 54:
                level2.branchFirstCurve = strtof(line.c_str(), NULL);
                continue;
            case 55:
                level2.branchFirstCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 56:
                level2.branchSecondCurve = strtof(line.c_str(), NULL);
                continue;
            case 57:
                level2.branchSecondCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 58:
                level2.initialUnusedSpace = strtof(line.c_str(), NULL);
                continue;
            case 59:
                level2.initialUnusedSpaceDelta = strtof(line.c_str(), NULL);
                continue;
            case 60:
                level2.branchSubdivisions = atoi(line.c_str());
                continue;
            case 61:
                level2.branchCount = atoi(line.c_str());
                continue;
            case 62:
                level2.branchCountDelta = atoi(line.c_str());
                continue;
            // Line #63-81: Level 3
            case 63:
                level3.branchLength = strtof(line.c_str(), NULL);
                continue;
            case 64:
                level3.branchLengthDelta = strtof(line.c_str(), NULL);
                continue;
            case 65:
                level3.branchBase = strtof(line.c_str(), NULL);
                continue;
            case 66:
                level3.branchBaseDelta = strtof(line.c_str(), NULL);
                continue;
            case 67:
                level3.branchTip = strtof(line.c_str(), NULL);
                continue;
            case 68:
                level3.branchTipDelta = strtof(line.c_str(), NULL);
                continue;
            case 69:
                level3.branchSplitAngle = strtof(line.c_str(), NULL);
                continue;
            case 70:
                level3.branchSplitAngleDelta = strtof(line.c_str(), NULL);
                continue;
            case 71:
                level3.branchCurvature = strtof(line.c_str(), NULL);
                continue;
            case 72:
                level3.branchCurvatureDelta = strtof(line.c_str(), NULL);
                continue;
            case 73:
                level3.branchFirstCurve = strtof(line.c_str(), NULL);
                continue;
            case 74:
                level3.branchFirstCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 75:
                level3.branchSecondCurve = strtof(line.c_str(), NULL);
                continue;
            case 76:
                level3.branchSecondCurveDelta = strtof(line.c_str(), NULL);
                continue;
            case 77:
                level3.initialUnusedSpace = strtof(line.c_str(), NULL);
                continue;
            case 78:
                level3.initialUnusedSpaceDelta = strtof(line.c_str(), NULL);
                continue;
            case 79:
                level3.branchSubdivisions = atoi(line.c_str());
                continue;
            case 80:
                level3.branchCount = atoi(line.c_str());
                continue;
            case 81:
                level3.branchCountDelta = atoi(line.c_str());
                continue;
            // Line #82-89 Branch Separation
            case 82:
                stump.branchSeparation = strtof(line.c_str(), NULL);
                continue;
            case 83:
                stump.branchSeparationDelta = strtof(line.c_str(), NULL);
                continue;
            case 84:
                level1.branchSeparation = strtof(line.c_str(), NULL);
                continue;
            case 85:
                level1.branchSeparationDelta = strtof(line.c_str(), NULL);
                continue;
            case 86:
                level2.branchSeparation = strtof(line.c_str(), NULL);
                continue;
            case 87:
                level2.branchSeparationDelta = strtof(line.c_str(), NULL);
                continue;
            case 88:
                level3.branchSeparation = strtof(line.c_str(), NULL);
                continue;
            case 89:
                level3.branchSeparationDelta = strtof(line.c_str(), NULL);
                continue;
                // Branch Quality
            case 90:
                branchQuality = atoi(line.c_str());
                continue;
            // Line 91-99 Leaf Settings
            case 91:
                leafType = atoi(line.c_str());
                continue;
            case 92:
                leafCount = atoi(line.c_str());
                continue;
            case 93:
                leafCountDelta = atoi(line.c_str());
                continue;
            case 94:
                leafSize = strtof(line.c_str(), NULL);
                continue;
            case 95:
                leafSizeDelta = strtof(line.c_str(), NULL);
                continue;
            case 96:
                leafSizeX = strtof(line.c_str(), NULL);
                continue;
            case 97:
                leafSizeXDelta = strtof(line.c_str(), NULL);
                continue;
            case 98:
                leafBend = strtof(line.c_str(), NULL);
                continue;
            case 99:
                leafBendDelta = strtof(line.c_str(), NULL);
                continue;
            case 100:
                leafZoom = strtof(line.c_str(), NULL);
                continue;
            case 101:
                minLeafLevel = atoi(line.c_str());
                continue;
            case 102:
                leafOffset = strtof(line.c_str(), NULL);
                continue;
            case 103:
                leafOffsetDelta = strtof(line.c_str(), NULL);
                continue;
        }
    }

    file.close();
    if (!startCheck) { printf("Session #%d Successfully Loaded.\n", selectedSession); }

    int sizes = allDrawables.size();

    bool sab = saveButton->selected;
    bool sb = sessionButton->selected;
    bool tb = textureButton->selected;
    for (int i = 0; i < sizes; i++){ allDrawables[i]->selected = false; }
    textureButton->selected = tb;
    sessionButton->selected = sb;
    saveButton->selected = sab;

    if (infoDisplayer != NULL) { infoDisplayer->ToggleInfo(); }
    if (inputBeingEdited != NULL) { inputBeingEdited->CancelEdit(); }

    // Highlight selected options
    HighlightSelections();
}

int Options::GetSeed() { return seed;  }
void Options::SetSeed(int newSeed) { seed = newSeed; SaveSession(); }

static void DrawText(float x,float y,void *font,char *string)
{
    char *c;
    glRasterPos2f(x, y);
    for (c=string; *c != '\0'; c++) { glutBitmapCharacter(font, *c); }
}

const int POPUP_WIDTH = 330;
const int POPUP_HEIGHT = 116;
static GLint infoBackgroundTex;
void Options::Update()
{
    glEnable(GL_BLEND); // Allow Semi Transparent Textures, But Only On The UI
    CameraHandler::SetOrthographicProjection();


    float startX = INITIAL_X_OFFSET + MENU_WIDTH;
	if (infoDisplayer != NULL)
    {
        float top = 0;
        float bottom = 65;
        Images::LoadTextureStart(infoBackgroundTex);
        glBegin (GL_QUADS);
            glTexCoord2f(0.02,0.02);
            glVertex2d(startX,bottom);
            glTexCoord2f(0.02,0.98);
            glVertex2d(startX, top);
            glTexCoord2f(0.98,0.98);
            glVertex2d(1920,top);
            glTexCoord2f(0.98,0.02);
            glVertex2d(1920,bottom);
        glEnd();
        Images::LoadTextureEnd();
    }

    startX = INITIAL_X_OFFSET + MENU_WIDTH + margin;
    glPushMatrix();
        glLoadIdentity();
        glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]);
        DrawText(startX, 20, helv18, desc1);
        DrawText(startX, 38, eight, desc2);
        DrawText(startX, 56, helv12, desc3);
        glColor3f(0,0,0);
        DrawText(startX, CameraHandler::CurrentH()-12,helv12,Utilities::Concatenate("(Current Seed: ", seed, ")"));
	glPopMatrix();

    // Draw All Non-Tab Items
    for (int i = 0; i < allDrawables.size(); i++) { glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]); allDrawables[i]->DrawSelf(); }

    // Draw In-Tab Menus
    for (int i = 0; i < menus.size(); i++) { glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]); menus[i]->DrawMenu(); }

    if (editStatus != NoMessage)
    {
        inputBeingEdited -> EditText();

        char* titleToUse;
        char* subtitleToUse;
        char* descToUse;
        char* desc2ToUse;
        char* desc3ToUse;
        bool draw = true;
        switch(editStatus)
        {
            case ListeningFloat:
            case ListeningInt:
                titleToUse = popupTitle;
                subtitleToUse = popupSubTitle;
                descToUse = popupDesc;
                desc2ToUse = popupSubDesc;
                desc3ToUse = popupSubSubDesc;
                break;
            case Error:
                titleToUse = "Invalid Input";
                subtitleToUse = "";
                descToUse = "The input you have provided is invalid.";
                desc2ToUse = "";
                desc3ToUse = errorText;
                break;
            default:
                titleToUse = "";
                subtitleToUse = "";
                descToUse = "";
                desc2ToUse = "";
                desc3ToUse = "";
                draw = false;
                break;
        }
        if (draw)
        {
            startX = float(glutGet(GLUT_WINDOW_WIDTH)/2.0) - (float(POPUP_WIDTH) / 2.0);
            float startY = float(glutGet(GLUT_WINDOW_HEIGHT)/2.0) - (float(POPUP_HEIGHT) / 2.0);
            float topY = startY+POPUP_HEIGHT;
            Images::LoadTextureStart(infoBackgroundTex);
            glBegin (GL_QUADS);
                glTexCoord2f(0.02,0.02);
                glVertex2d(startX,startY);
                glTexCoord2f(0.02,0.98);
                glVertex2d(startX,topY);
                glTexCoord2f(0.98,0.98);
                glVertex2d(startX+POPUP_WIDTH,topY);
                glTexCoord2f(0.98,0.02);
                glVertex2d(startX+POPUP_WIDTH,startY);
            glEnd();
            Images::LoadTextureEnd();

            startX += margin;
            startY += margin;
            topY += margin;
            glPushMatrix();
                glLoadIdentity();
                glColor3f(MENU_WHITE[0],MENU_WHITE[1],MENU_WHITE[2]);
                DrawText(startX, startY + 20, helv18, titleToUse);
                DrawText(startX, startY + 38, helv12, subtitleToUse);
                DrawText(startX, startY + 56, helv12, descToUse);
                DrawText(startX, startY + 74, helv12, desc2ToUse);
                DrawText(startX, startY + 92, helv12, desc3ToUse);
            glPopMatrix();
        }
    }

	CameraHandler::RestorePerspectiveProjection();
    glDisable(GL_BLEND);
}

static void AttemptToCloseError()
{
    if (editStatus != Error) { return; }

    editStatus = NoMessage;
    inputBeingEdited = NULL;
}

void Options::PressNormalKey(unsigned char key, int x, int y)
{
    AttemptToCloseError();

    if (inputBeingEdited == NULL || (editStatus != ListeningInt
        && editStatus != ListeningFloat)) { return; }

    switch(key)
    {
        case '0':
            if (editInput.size() == 0) { return; }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            editInput.push_back(key);
            break;
        case '.':
            if (usedDot || editStatus != ListeningFloat) { return; }
            usedDot = true;
            editInput.push_back(key);
            break;
        case '-':
            if (editInput.size() > 0 || usedMinus) { return; }
            usedMinus = true;
            editInput.push_back(key);
            break;
        case 13: // ENTER
            inputBeingEdited->AttemptToAcceptInput();
            break;
        case 27: // ESCAPE
            inputBeingEdited->CancelEdit();
            break;
        case 8: // BACKSPACE
            if (editInput.size() == 0) { return; }
            if (editInput[editInput.size()-1] == '.') { usedDot = false; }
            if (editInput[editInput.size()-1] == '-') { usedMinus = false; }
            editInput.pop_back();
            break;
    }
}

void Options::ReleaseNormalKey(unsigned char key, int x, int y)
{

}

void Options::PressSpecialKey(int key, int x, int y)
{
    AttemptToCloseError();
}

void Options::ReleaseSpecialKey(int key, int x, int y)
{


}

void Options::OnResize(int w, int h)
{
    for (int i = 0; i < allDrawables.size(); i++) { allDrawables[i]->RecalculateClickAreas(w,h); }
    for (int i = 0; i < menus.size(); i++) { menus[i]->RecalculateClickAreas(w,h); }
}

void Options::MouseClick(int button, int state, int x, int y)
{
    AttemptToCloseError();

    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            if (state != GLUT_DOWN) { return; }
            for (int i = 0; i < allDrawables.size(); i++)
            {
                Drawable* next = allDrawables[i];
                if (next->IsClickValid(x, y))
                {
                    switch(next->action)
                    {
                        case ChangeSession:
                            sessionButtons[selectedSession-1]->selected = false;
                            selectedSession = next->id;
                            SaveOptions();
                            LoadSession();
                            sessionButtons[selectedSession-1]->selected = true;
                            break;
                        case SaveModel:
                            Shapes::SaveModel();
                            saveButton->ToggleVisibility();
                            break;
                        case ToggleWireframe:
                            renderWireframe = !renderWireframe;
                            wireframeButton->selected = renderWireframe;
                            SaveOptions();
                            break;
                        case ToggleSave:
                            saveButton->ToggleVisibility();
                            break;
                        case ChangeTab:
                            selectedTab = next->id;
                            for (int i = 0; i < menus.size(); i++) { menus[i]->OnTabChange(next->id); }
                            HighlightSelections();
                            if (infoDisplayer != NULL) { infoDisplayer->ToggleInfo(); }
                            break;
                        case ChangeTexture:
                            int target = next->id % 3;
                            int type = next->id/3;
                            switch(type)
                            {
                                case 0:
                                    treeTextureButtons[treeTexture]->selected = false;
                                    treeTexture = target;
                                    treeTextureButtons[treeTexture]->selected = true;
                                    break;
                                case 1:
                                    leavesTextureButtons[leavesTexture]->selected = false;
                                    leavesTexture = target;
                                    leavesTextureButtons[leavesTexture]->selected = true;
                                    break;
                                case 2:
                                    groundTextureButtons[groundTexture]->selected = false;
                                    groundTexture = target;
                                    groundTextureButtons[groundTexture]->selected = true;
                                    break;
                            }
                            SaveSession();
                            break;
                    }
                }
            }
            for (int i = 0; i < menus.size(); i++) { menus[i]->AttemptToClick(x, y); }
            break;
        case GLUT_RIGHT_BUTTON:
            if (state != GLUT_DOWN) { return; }

            break;
    }
}

void Options::Start()
{
    infoBackgroundTex = Images::GetImageTextureFromFilePath("UI\\Darker.png");

    int logoSizeY = 120;
    int logoSizeX = 70;
    allDrawables =
    {
        new Button(1,1,-margin+1,-margin*3, logoSizeX, logoSizeY, TopRight,DoNothing, Images::GetImageTextureFromFilePath("UI\\Mokuzai.png"), Images::GetImageTextureFromFilePath("UI\\Mokuzai.png")),
        new Button(0,1,0,0,INITIAL_X_OFFSET, 1080, TopLeft, DoNothing, Images::GetImageTextureFromFilePath("UI\\Darkest.png"),Images::GetImageTextureFromFilePath("UI\\Darkest.png")),
        new Button(0,1,INITIAL_X_OFFSET,0, MENU_WIDTH, 1080, TopLeft, DoNothing, Images::GetImageTextureFromFilePath("UI\\Dark.png"),Images::GetImageTextureFromFilePath("UI\\Dark.png")),
    };

    tabs =
    {
        // Bottom Tabs
        new Tab(0,0, margin, (spacing * 0) + margin + (tabSize * 0), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Br_3.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Br_3_Selected.png"), 3),
        new Tab(0,0, margin, (spacing * 1) + margin + (tabSize * 1), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Br_2.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Br_2_Selected.png"), 2),
        new Tab(0,0, margin, (spacing * 2) + margin + (tabSize * 2), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Br_1.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Br_1_Selected.png"), 1),
        new Tab(0,0, margin, (spacing * 3) + margin + (tabSize * 3), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Leaves.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Leaves_Selected.png"), 4),
        new Tab(0,0, margin, (spacing * 4) + margin + (tabSize * 4), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Tree.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Tree_Selected.png"), 0),
        new Tab(0,0, margin, (spacing * 5) + margin + (tabSize * 5), tabSize, tabSize, BottomLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_General.png"), Images::GetImageTextureFromFilePath("UI\\Tab_General_Selected.png"), 10),

        // Top Tab
        new Tab(0,1, margin, - (margin*3), tabSize, tabSize, TopLeft, ChangeTab, Images::GetImageTextureFromFilePath("UI\\Tab_Info.png"), Images::GetImageTextureFromFilePath("UI\\Tab_Info_Selected.png"), 11)
     };
     for (int i = 0; i < tabs.size(); i++) { allDrawables.push_back(tabs[i]); }


    float sessionY =  -margin*6 - logoSizeY - spacing;
    sessionButtons =
    {
        // Session Buttons
        new Button(1,1, -(spacing * 3) - margin - (tabSize * 3), sessionY, tabSize, tabSize, TopRight, ChangeSession, Images::GetImageTextureFromFilePath("UI\\Button_1.png"), Images::GetImageTextureFromFilePath("UI\\Button_1_Selected.png"), 1),
        new Button(1,1, -(spacing * 2) - margin - (tabSize * 2), sessionY, tabSize, tabSize, TopRight, ChangeSession, Images::GetImageTextureFromFilePath("UI\\Button_2.png"), Images::GetImageTextureFromFilePath("UI\\Button_2_Selected.png"), 2),
        new Button(1,1, -(spacing * 1) - margin - (tabSize * 1), sessionY, tabSize, tabSize, TopRight, ChangeSession, Images::GetImageTextureFromFilePath("UI\\Button_3.png"), Images::GetImageTextureFromFilePath("UI\\Button_3_Selected.png"), 3),
    };
    for (int i = 0; i < sessionButtons.size(); i++) { allDrawables.push_back(sessionButtons[i]); }
    sessionButton =  new ToggleButton(1,1, -margin, sessionY, tabSize, tabSize, TopRight, DoNothing, Images::GetImageTextureFromFilePath("UI\\Button_Settings.png"), Images::GetImageTextureFromFilePath("UI\\Button_Settings_Selected.png"), sessionButtons, 1);
    allDrawables.push_back(sessionButton);

    saveButtons =
    {
        // Session Buttons
        new Button(1,1, -(spacing * 2) - margin - (tabSize * 2), sessionY - tabSize - spacing, tabSize, tabSize, TopRight, SaveModel, Images::GetImageTextureFromFilePath("UI\\Button_Confirm.png"), Images::GetImageTextureFromFilePath("UI\\Button_Confirm.png"), 1),
        new Button(1,1, -(spacing * 1) - margin - (tabSize * 1), sessionY - tabSize - spacing, tabSize, tabSize, TopRight, ToggleSave, Images::GetImageTextureFromFilePath("UI\\Button_Cancel.png"), Images::GetImageTextureFromFilePath("UI\\Button_Cancel.png"), 0),
    };
    for (int i = 0; i < saveButtons.size(); i++) { allDrawables.push_back(saveButtons[i]); }
    saveButton = new ToggleButton(1,1, -margin, sessionY - tabSize - spacing, tabSize, tabSize, TopRight, ToggleSubMenu, Images::GetImageTextureFromFilePath("UI\\Button_Save.png"), Images::GetImageTextureFromFilePath("UI\\Button_Save_Selected.png"), saveButtons, 2);
    allDrawables.push_back(saveButton);

    wireframeButton = new Button(1,1, -margin, sessionY - tabSize*2 - spacing*2, tabSize, tabSize, TopRight, ToggleWireframe, Images::GetImageTextureFromFilePath("UI\\Button_Solid.png"), Images::GetImageTextureFromFilePath("UI\\Button_Wireframe.png"), 2);
    allDrawables.push_back(wireframeButton);

    vector<Button*> textureButtons;
    treeTextureButtons =
    {
        // Tree Texture Buttons
        new Button(1,0, -(spacing * 3) - margin - (tabSize * 3), margin + spacing + tabSize, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_1.png"), Images::GetImageTextureFromFilePath("UI\\Button_1_Selected.png"), 0),
        new Button(1,0, -(spacing * 2) - margin - (tabSize * 2), margin + spacing + tabSize, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_2.png"), Images::GetImageTextureFromFilePath("UI\\Button_2_Selected.png"), 1),
        new Button(1,0, -(spacing * 1) - margin - (tabSize * 1), margin + spacing + tabSize, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_3.png"), Images::GetImageTextureFromFilePath("UI\\Button_3_Selected.png"), 2),
        new Button(1,0, -(spacing * 4) - margin - (tabSize * 4), margin + spacing + tabSize, tabSize, tabSize, BottomRight, DoNothing, Images::GetImageTextureFromFilePath("UI\\Tree.png"), Images::GetImageTextureFromFilePath("UI\\Tree.png"), 0),
     };
     for (int i = 0; i < treeTextureButtons.size(); i++) { allDrawables.push_back(treeTextureButtons[i]); textureButtons.push_back(treeTextureButtons[i]); }

    leavesTextureButtons =
    {
        // Leaves Texture Buttons
        new Button(1,0, -(spacing * 3) - margin - (tabSize * 3), margin, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_1.png"), Images::GetImageTextureFromFilePath("UI\\Button_1_Selected.png"), 3),
        new Button(1,0, -(spacing * 2) - margin - (tabSize * 2), margin, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_2.png"), Images::GetImageTextureFromFilePath("UI\\Button_2_Selected.png"), 4),
        new Button(1,0, -(spacing * 1) - margin - (tabSize * 1), margin, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_3.png"), Images::GetImageTextureFromFilePath("UI\\Button_3_Selected.png"), 5),
        new Button(1,0, -(spacing * 4) - margin - (tabSize * 4), margin, tabSize, tabSize, BottomRight, DoNothing, Images::GetImageTextureFromFilePath("UI\\Leaves.png"), Images::GetImageTextureFromFilePath("UI\\Leaves.png"), 0),
       };
     for (int i = 0; i < leavesTextureButtons.size(); i++) { allDrawables.push_back(leavesTextureButtons[i]); textureButtons.push_back(leavesTextureButtons[i]);  }

    groundTextureButtons =
    {
        // Ground Texture Button
        new Button(1,0, -(spacing * 3) - margin - (tabSize * 3), margin + spacing*2 + tabSize*2, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_1.png"), Images::GetImageTextureFromFilePath("UI\\Button_1_Selected.png"), 6),
        new Button(1,0, -(spacing * 2) - margin - (tabSize * 2), margin + spacing*2 + tabSize*2, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_2.png"), Images::GetImageTextureFromFilePath("UI\\Button_2_Selected.png"), 7),
        new Button(1,0, -(spacing * 1) - margin - (tabSize * 1), margin + spacing*2 + tabSize*2, tabSize, tabSize, BottomRight, ChangeTexture, Images::GetImageTextureFromFilePath("UI\\Button_3.png"), Images::GetImageTextureFromFilePath("UI\\Button_3_Selected.png"), 8),
        new Button(1,0, -(spacing * 4) - margin - (tabSize * 4), margin + spacing*2 + tabSize*2, tabSize, tabSize, BottomRight, DoNothing, Images::GetImageTextureFromFilePath("UI\\Ground.png"), Images::GetImageTextureFromFilePath("UI\\Ground.png"), 0),
     };
     for (int i = 0; i < groundTextureButtons.size(); i++) { allDrawables.push_back(groundTextureButtons[i]); textureButtons.push_back(groundTextureButtons[i]); }

     textureButton = new ToggleButton(1,0, -margin, margin, tabSize, tabSize, BottomRight, DoNothing, Images::GetImageTextureFromFilePath("UI\\Button_Textures.png"), Images::GetImageTextureFromFilePath("UI\\Button_Textures_Selected.png"), textureButtons, 0);
     allDrawables.push_back(textureButton);

    char* treeLevelsName = "Tree Levels";
    char* treeLevelsDesc = "Indicates how many child branches each branch can have. (0 = Only Stump)";

    char* branchQualityName = "Branch Quality";
    char* branchQualityDesc = "Indicates the number of vertex the circles that shape each branch will use.";
    int branchQualityMin = 3;
    int branchQualityMax = 32;

    char* stumpSizeName = "Stump Size";
    char* stumpSizeDesc = "Indicates the stump height for the tree, which itself is a base size for the whole tree.";
    float stumpSizeMin = 0.1;
    float stumpSizeMax = 10;
    char* stumpSizeDeltaName = "(Variation)";
    char* stumpSizeDeltaDesc = "Indicates an amount by which \"Stump Size\" can vary depending on the seed.";
    float stumpSizeDeltaMin = 0;
    float stumpSizeDeltaMax = 10;
    char* stumpBaseName = "Stump Base Size";
    char* stumpBaseDesc = "Indicates the radio of the base of the tree. This will contribute greatly to the thickness of branches.";
    float stumpBaseSizeMin = 0.01f;
    float stumpBaseSizeMax = 2;
    char* stumpBaseDeltaName = "(Variation)";
    char* stumpBaseDeltaDesc = "Indicates an amount by which \"Stump Base Size\" can vary depending on the seed.";
    float stumpBaseDeltaMin = 0;
    float stumpBaseDeltaMax = 2;

    char* branchLengthName = "Branch Length"; // NOT FOR STUMP
    char* branchLengthDesc = "Indicates a % that determines the length of the branch depending on the height of the previous branch (for the first branch level this is the stump)."; // NOT FOR STUMP
    float branchLengthMin = 0.1;
    float branchLengthMax = 3;
    char* branchLengthDeltaName = "(Variation)"; // NOT FOR STUMP
    char* branchLengthDeltaDesc = "Indicates a % by which \"Branch Length\" can vary depending on the seed."; // NOT FOR STUMP
    float branchLengthDeltaMin = 0;
    float branchLengthDeltaMax = 2;
    char* branchBaseName = "Branch Base Size"; // NOT FOR STUMP
    char* branchBaseDesc = "Indicates a % that determines the length of the branch depending on the radio of the previous branch (for the first branch level this is the stump).";
    float branchBaseMin = 0.1;
    float branchBaseMax = 2;
    char* branchBaseDeltaName = "(Variation)";
    char* branchBaseDeltaDesc = "Indicates a % by which \"Branch Base Size\" can vary depending on the seed.";
    float branchBaseDeltaMin = 0;
    float branchBaseDeltaMax = 2;

    char* tipName = "Tip Size";
    char* tipDesc = "Indicates a % that determines the radio of the tip depending on the base size. This greatly affects branches.";
    float tipMin = 0.01;
    float tipMax = 1;
    char* tipDeltaName = "(Variation)";
    char* tipDeltaDesc = "Indicates a % by which \"Tip Size\" can vary depending on the seed.";
    float tipDeltaMin = 0;
    float tipDeltaMax = 1;
    char* branchSplitAngleName = "Base Angle";
    char* branchSplitAngleDesc = "Gives an initial angle by which the WHOLE branch/stump will be tilted by.";
    float branchSplitAngleMin = -180;
    float branchSplitAngleMax = 180;
    char* branchSplitAngleDeltaName = "(Variation)";
    char* branchSplitAngleDeltaDesc = "Indicates a degree amount by which \"Base Angle\" can vary depending on the seed.";
    float branchSplitAngleDeltaMin = 0;
    float branchSplitAngleDeltaMax = 360;
    char* branchCurvatureName = "Curvature";
    char* branchCurvatureDesc = "Indicates a degree amount that determines the \"shape\" a branch will have. i.e. a 90 degree branch will be like an inverse L, and 180 an I.";
    float branchCurvatureMin = -180;
    float branchCurvatureMax = 180;
    char* branchCurvatureDeltaName = "(Variation)";
    char* branchCurvatureDeltaDesc = "Indicates a degree amount by which \"Curvature\" can vary depending on the seed.";
    float branchCurvatureDeltaMin = 0;
    float branchCurvatureDeltaMax = 360;

    char* initialUnusedSpaceName = "Initial Unused Space";
    char* initialUnusedSpaceDesc = "Indicates a % at where branches cannot grow, calculated from base to tip.";
    float initialUnusedSpaceMin = 0.1;
    float initialUnusedSpaceMax = 0.95;
    char* initialUnusedSpaceDeltaName = "(Variation)";
    char* initialUnusedSpaceDeltaDesc = "Indicates a % by which \"Initial Unused Space\" can vary depending on the seed. This cannot cause going under the minimum or over the maximum.";
    float initialUnusedSpaceDeltaMin = 0;
    float initialUnusedSpaceDeltaMax = 0.95;

    char* branchSeparationName = "Branch Separation";
    char* branchSeparationDesc = "Indicates an angle by which each branch will be rotated clockwise relative to the last branch of the same level before being added. ";
    float branchSeparationMin = -360;
    float branchSeparationMax = 360;
    char* branchSeparationDeltaName = "(Variation)";
    char* branchSeparationDeltaDesc = "Indicates a degree amount by which \"Branch Separation Angle\" can vary depending on the seed.";
    float branchSeparationDeltaMin = 0;
    float branchSeparationDeltaMax = 360;

    char* branchSubdivisionsName = "Subdivisions Amount";
    char* branchSubdivisionsDesc = "Indicates the number of segments each branch will be divided by to form its curves.";
    int branchSubdivisionsMin = 1;
    int branchSubdivisionsMax = 7;
    char* branchCountName = "Branch Count";
    char* branchCountDesc = "Indicates the amount of branches this level will have.";
    int branchCountMin = 1;
    int branchCountMax = 10;
    char* branchCountDeltaName = "(Variation)";
    char* branchCountDeltaDesc = "Indicates the amount of branches that a branch of this level can randomly add/remove to those marked in \"Branch Count\" depending on the seed.";
    int branchCountDeltaMin = 0;
    int branchCountDeltaMax = 10;

    char* minLeafLevelName = "Min. Leaf Level";
    char* minLeafLevelDesc = "Indicates the minimum level required for something to start producing leaves. (Level 0 = Stump)";
    int minLeafLevelMin = 0;
    int minLeafLevelMax = 3;

    char* leafTypeName = "Leaf Type";
    char* leafTypeDesc = "0 = None, 1 = Flat Rectangle, 2 = Flat Droplet, 3 = Crooked Droplet";
    int leafTypeMin = 0;
    int leafTypeMax = 3;

    char* leafCountName = "Leaf Count";
    char* leafCountDesc = "Determines the amount of leaves that will be found on the topmost level branches.";
    int leafCountMin = 0;
    int leafCountMax = 100;

    char* leafCountDeltaName = "(Variation)";
    char* leafCountDeltaDesc = "Indicates a number amount by which \"Leaf Count\" can vary depending on the seed on a per branch basis.";
    int leafCountDeltaMin = 0;
    int leafCountDeltaMax = 100;

    char* leafSizeName = "Leaf Size";
    char* leafSizeDesc = "Determines the length of each leaf in the same units as the stump.";
    float leafSizeMin = 0.01;
    float leafSizeMax = 4;

    char* leafSizeDeltaName = "(Variation)";
    char* leafSizeDeltaDesc = "Indicates a unit amount by which \"Leaf Size\" can vary depending on the seed.";
    float leafSizeDeltaMin = 0;
    float leafSizeDeltaMax = 4;

    char* leafSizeXName = "Leaf Size X";
    char* leafSizeXDesc = "Determines the width of each leaf based on the length of the same leaf.";
    float leafSizeXMin = 0.01;
    float leafSizeXMax = 2;

    char* leafSizeXDeltaName = "(Variation)";
    char* leafSizeXDeltaDesc = "Indicates a % amount by which \"Leaf Size X\" can vary depending on the seed.";
    float leafSizeXDeltaMin = 0;
    float leafSizeXDeltaMax = 2;

    char* leafBendName = "Leaf Bend";
    char* leafBendDesc = "Indicates a degree amount by which a leaf will look towards or away of the base.";
    float leafBendMin = -90;
    float leafBendMax = 90;

    char* leafBendDeltaName = "(Variation)";
    char* leafBendDeltaDesc = "Indicates a degree amount by which \"Leaf Bend\" can vary depending on the seed.";
    float leafBendDeltaMin = 0;
    float leafBendDeltaMax = 180;

    char* leafZoomName = "Leaf Zoom";
    char* leafZoomDesc = "Indicates how many leaves side by side are required to use the entire leaf texture.";
    int leafZoomMin = 1;
    int leafZoomMax = 1000;

    char* leafOffsetName = "Leaf Offset";
    char* leafOffsetDesc = "A value that indicates how much leaves can \"float\" above the branch they belong to.";
    float leafOffsetMin = 0;
    float leafOffsetMax = 3;

    char* leafOffsetDeltaName = "(Variation)";
    char* leafOffsetDeltaDesc = "Indicates a unit amount by which \"Leaf Offset\" can vary depending on the seed.";
    float leafOffsetDeltaMin = 0;
    float leafOffsetDeltaMax = 3;

    int fontSpacing = 16;
    int deltaSpace = 5;
    menus =
    {
        // Info Tab
        new Menu(11, {
            // Mokuzai White Logo
            new MenuItem(0.5, 1, -float(logoSizeX)/2.0f, -margin*3, logoSizeX, logoSizeY, TopLeft, DoNothing, Images::GetImageTextureFromFilePath("UI\\Mokuzai_White.png")),

            // Creators
            new TextEntry(0,1,65, -margin*4 - spacing - logoSizeY - fontSpacing, "Mokuzai", TopLeft, helv18),
            new TextEntry(0,1,60, -margin*4 - spacing*2 - logoSizeY - fontSpacing*2, "Tree Generator", TopLeft, helv12),
            new TextEntry(0,1,60, -margin*6 - spacing*3 - logoSizeY - fontSpacing*3, "Made By:", TopLeft, helv18),
            new TextEntry(0,1,66, -margin*6 - spacing*4 - logoSizeY - fontSpacing*4, "Luis Arzola", TopLeft, helv12),
            new TextEntry(0,1,63, -margin*6 - spacing*5 - logoSizeY - fontSpacing*6, "Spring 2020", TopLeft, helv12),

             // Controls
            new TextEntry(0,0,margin, margin*2 + fontSpacing*7, "[PgDn][PgUp] - Change Seed", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin*2 + fontSpacing*6, "[Ins] - Reset Rotation", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin*2 + fontSpacing*5, "[Del] - Reset Position", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin*2 + fontSpacing*4, "[CTRL] + [Up][Down] - Move", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin*2 + fontSpacing*3, "[Arrow Keys] - Move + Zoom", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin + fontSpacing*2, "[MMB] - Move", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin + fontSpacing*1, "[Mouse Wheel] - Zoom", BottomLeft, helv12),
            new TextEntry(0,0,margin, margin + fontSpacing*0, "[RMB] - Rotate Tree", BottomLeft, helv12),
        }),
        // General Tab
        new Menu(10, {
            new InputValue<int>(0, 0, 3, treeLevelsName, treeLevelsDesc, []()->int{ return maxLevels; }, [](int newValue)->void { maxLevels = newValue; } ),
            new InputValue<int>((spacing + INPUT_HEIGHT)*1, branchQualityMin, branchQualityMax, branchQualityName, branchQualityDesc, []()->int{ return branchQuality; }, [](int newValue)->void { branchQuality = newValue; } ),
        }),
        // Stump
        new Menu(0, {
            new InputValue<float>(0, stumpSizeMin, stumpSizeMax, stumpSizeName, stumpSizeDesc, []()->float{ return stump.branchLength; },
            [](float newValue)->void { stump.branchLength = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*1, stumpSizeDeltaMin, stumpSizeDeltaMax, stumpSizeDeltaName, stumpSizeDeltaDesc,
                                  []()->float{ return stump.branchLengthDelta; }, [](float newValue)->void { stump.branchLengthDelta = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*2, stumpBaseSizeMin, stumpBaseSizeMax, stumpBaseName, stumpBaseDesc,
                                  []()->float{ return stump.branchBase; }, [](float newValue)->void { stump.branchBase = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*3, stumpBaseDeltaMin, stumpBaseDeltaMax, stumpBaseDeltaName, stumpBaseDeltaDesc,
                                  []()->float{ return stump.branchBaseDelta; }, [](float newValue)->void { stump.branchBaseDelta = newValue; } ),
            // Generic
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*4, tipMin, tipMax, tipName, tipDesc,
                                  []()->float{ return stump.branchTip; }, [](float newValue)->void { stump.branchTip = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*5, tipDeltaMin, tipDeltaMax, tipDeltaName, tipDeltaDesc,
                                  []()->float{ return stump.branchTipDelta; }, [](float newValue)->void { stump.branchTipDelta = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*6, branchSplitAngleMin, branchSplitAngleMax, branchSplitAngleName, branchSplitAngleDesc,
                                  []()->float{ return stump.branchSplitAngle; }, [](float newValue)->void { stump.branchSplitAngle = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*7, branchSplitAngleDeltaMin, branchSplitAngleDeltaMax, branchSplitAngleDeltaName, branchSplitAngleDeltaDesc,
                                  []()->float{ return stump.branchSplitAngleDelta; }, [](float newValue)->void { stump.branchSplitAngleDelta = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*8, branchCurvatureMin, branchCurvatureMax, branchCurvatureName, branchCurvatureDesc,
                                  []()->float{ return stump.branchCurvature; }, [](float newValue)->void { stump.branchCurvature = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*9, branchCurvatureDeltaMin, branchCurvatureDeltaMax, branchCurvatureDeltaName, branchCurvatureDeltaDesc,
                                  []()->float{ return stump.branchCurvatureDelta; }, [](float newValue)->void { stump.branchCurvatureDelta = newValue; } ),
            new InputValue<float>(deltaSpace*5 + (spacing + INPUT_HEIGHT)*10, initialUnusedSpaceMin, initialUnusedSpaceMax, initialUnusedSpaceName, initialUnusedSpaceDesc,
                                  []()->float{ return stump.initialUnusedSpace; }, [](float newValue)->void { stump.initialUnusedSpace = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*11, initialUnusedSpaceDeltaMin, initialUnusedSpaceDeltaMax, initialUnusedSpaceDeltaName, initialUnusedSpaceDeltaDesc,
                                  []()->float{ return stump.initialUnusedSpaceDelta; }, [](float newValue)->void { stump.initialUnusedSpaceDelta = newValue; } ),
            new InputValue<float>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*12, branchSeparationMin, branchSeparationMax, branchSeparationName, branchSeparationDesc,
                                  []()->float{ return stump.branchSeparation; }, [](float newValue)->void { stump.branchSeparation = newValue; } ),
            new InputValue<float>(deltaSpace*5 + (spacing + INPUT_HEIGHT)*13, branchSeparationDeltaMin, branchSeparationDeltaMax, branchSeparationDeltaName, branchSeparationDeltaDesc,
                                  []()->float{ return stump.branchSeparationDelta; }, [](float newValue)->void { stump.branchSeparationDelta = newValue; } ),

            new InputValue<int>(deltaSpace*7 + (spacing + INPUT_HEIGHT)*14, branchSubdivisionsMin, branchSubdivisionsMax, branchSubdivisionsName, branchSubdivisionsDesc,
                                []()->int{ return stump.branchSubdivisions; }, [](int newValue)->void { stump.branchSubdivisions = newValue; } ),
            new InputValue<int>(deltaSpace*8 + (spacing + INPUT_HEIGHT)*15, branchCountMin, branchCountMax, branchCountName, branchCountDesc,
                                []()->int{ return stump.branchCount; }, [](int newValue)->void { stump.branchCount = newValue; } ),
            new InputValue<int>(deltaSpace*7 + (spacing + INPUT_HEIGHT)*16, branchCountDeltaMin, branchCountDeltaMax, branchCountDeltaName, branchCountDeltaDesc,
                                []()->int{ return stump.branchCountDelta; }, [](int newValue)->void { stump.branchCountDelta = newValue; } ),
        }),
        // Branch Level 1
        new Menu(1, {
            new InputValue<float>(0, branchLengthMin, branchLengthMax, branchLengthName, branchLengthDesc, []()->float{ return level1.branchLength; },
            [](float newValue)->void { level1.branchLength = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*1, branchLengthDeltaMin, branchLengthDeltaMax, branchLengthDeltaName, branchLengthDeltaDesc,
                                  []()->float{ return level1.branchLengthDelta; }, [](float newValue)->void { level1.branchLengthDelta = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*2, tipMin, tipMax, tipName, tipDesc,
                                  []()->float{ return level1.branchTip; }, [](float newValue)->void { level1.branchTip = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*3, tipDeltaMin, tipDeltaMax, tipDeltaName, tipDeltaDesc,
                                  []()->float{ return level1.branchTipDelta; }, [](float newValue)->void { level1.branchTipDelta = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*4, branchSplitAngleMin, branchSplitAngleMax, branchSplitAngleName, branchSplitAngleDesc,
                                  []()->float{ return level1.branchSplitAngle; }, [](float newValue)->void { level1.branchSplitAngle = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*5, branchSplitAngleDeltaMin, branchSplitAngleDeltaMax, branchSplitAngleDeltaName, branchSplitAngleDeltaDesc,
                                  []()->float{ return level1.branchSplitAngleDelta; }, [](float newValue)->void { level1.branchSplitAngleDelta = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*6, branchCurvatureMin, branchCurvatureMax, branchCurvatureName, branchCurvatureDesc,
                                  []()->float{ return level1.branchCurvature; }, [](float newValue)->void { level1.branchCurvature = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*7, branchCurvatureDeltaMin, branchCurvatureDeltaMax, branchCurvatureDeltaName, branchCurvatureDeltaDesc,
                                  []()->float{ return level1.branchCurvatureDelta; }, [](float newValue)->void { level1.branchCurvatureDelta = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*8, initialUnusedSpaceMin, initialUnusedSpaceMax, initialUnusedSpaceName, initialUnusedSpaceDesc,
                                  []()->float{ return level1.initialUnusedSpace; }, [](float newValue)->void { level1.initialUnusedSpace = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*9, initialUnusedSpaceDeltaMin, initialUnusedSpaceDeltaMax, initialUnusedSpaceDeltaName, initialUnusedSpaceDeltaDesc,
                                  []()->float{ return level1.initialUnusedSpaceDelta; }, [](float newValue)->void { level1.initialUnusedSpaceDelta = newValue; } ),
            new InputValue<float>(deltaSpace*5 + (spacing + INPUT_HEIGHT)*10, branchSeparationMin, branchSeparationMax, branchSeparationName, branchSeparationDesc,
                                  []()->float{ return level1.branchSeparation; }, [](float newValue)->void { level1.branchSeparation = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*11, branchSeparationDeltaMin, branchSeparationDeltaMax, branchSeparationDeltaName, branchSeparationDeltaDesc,
                                  []()->float{ return level1.branchSeparationDelta; }, [](float newValue)->void { level1.branchSeparationDelta = newValue; } ),

            new InputValue<int>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*12, branchSubdivisionsMin, branchSubdivisionsMax, branchSubdivisionsName, branchSubdivisionsDesc,
                                []()->int{ return level1.branchSubdivisions; }, [](int newValue)->void { level1.branchSubdivisions = newValue; } ),
            new InputValue<int>(deltaSpace*7 + (spacing + INPUT_HEIGHT)*13, branchCountMin, branchCountMax, branchCountName, branchCountDesc,
                                []()->int{ return level1.branchCount; }, [](int newValue)->void { level1.branchCount = newValue; } ),
            new InputValue<int>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*14, branchCountDeltaMin, branchCountDeltaMax, branchCountDeltaName, branchCountDeltaDesc,
                                []()->int{ return level1.branchCountDelta; }, [](int newValue)->void { level1.branchCountDelta = newValue; } ),
        }),
        // Branch Level 2
        new Menu(2, {
            new InputValue<float>(0, branchLengthMin, branchLengthMax, branchLengthName, branchLengthDesc, []()->float{ return level2.branchLength; },
            [](float newValue)->void { level2.branchLength = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*1, branchLengthDeltaMin, branchLengthDeltaMax, branchLengthDeltaName, branchLengthDeltaDesc,
                                  []()->float{ return level2.branchLengthDelta; }, [](float newValue)->void { level2.branchLengthDelta = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*2, tipMin, tipMax, tipName, tipDesc,
                                  []()->float{ return level2.branchTip; }, [](float newValue)->void { level2.branchTip = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*3, tipDeltaMin, tipDeltaMax, tipDeltaName, tipDeltaDesc,
                                  []()->float{ return level2.branchTipDelta; }, [](float newValue)->void { level2.branchTipDelta = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*4, branchSplitAngleMin, branchSplitAngleMax, branchSplitAngleName, branchSplitAngleDesc,
                                  []()->float{ return level2.branchSplitAngle; }, [](float newValue)->void { level2.branchSplitAngle = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*5, branchSplitAngleDeltaMin, branchSplitAngleDeltaMax, branchSplitAngleDeltaName, branchSplitAngleDeltaDesc,
                                  []()->float{ return level2.branchSplitAngleDelta; }, [](float newValue)->void { level2.branchSplitAngleDelta = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*6, branchCurvatureMin, branchCurvatureMax, branchCurvatureName, branchCurvatureDesc,
                                  []()->float{ return level2.branchCurvature; }, [](float newValue)->void { level2.branchCurvature = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*7, branchCurvatureDeltaMin, branchCurvatureDeltaMax, branchCurvatureDeltaName, branchCurvatureDeltaDesc,
                                  []()->float{ return level2.branchCurvatureDelta; }, [](float newValue)->void { level2.branchCurvatureDelta = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*8, initialUnusedSpaceMin, initialUnusedSpaceMax, initialUnusedSpaceName, initialUnusedSpaceDesc,
                                  []()->float{ return level2.initialUnusedSpace; }, [](float newValue)->void { level2.initialUnusedSpace = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*9, initialUnusedSpaceDeltaMin, initialUnusedSpaceDeltaMax, initialUnusedSpaceDeltaName, initialUnusedSpaceDeltaDesc,
                                  []()->float{ return level2.initialUnusedSpaceDelta; }, [](float newValue)->void { level2.initialUnusedSpaceDelta = newValue; } ),
            new InputValue<float>(deltaSpace*5 + (spacing + INPUT_HEIGHT)*10, branchSeparationMin, branchSeparationMax, branchSeparationName, branchSeparationDesc,
                                  []()->float{ return level2.branchSeparation; }, [](float newValue)->void { level2.branchSeparation = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*11, branchSeparationDeltaMin, branchSeparationDeltaMax, branchSeparationDeltaName, branchSeparationDeltaDesc,
                                  []()->float{ return level2.branchSeparationDelta; }, [](float newValue)->void { level2.branchSeparationDelta = newValue; } ),

            new InputValue<int>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*12, branchSubdivisionsMin, branchSubdivisionsMax, branchSubdivisionsName, branchSubdivisionsDesc,
                                []()->int{ return level2.branchSubdivisions; }, [](int newValue)->void { level2.branchSubdivisions = newValue; } ),
            new InputValue<int>(deltaSpace*7 + (spacing + INPUT_HEIGHT)*13, branchCountMin, branchCountMax, branchCountName, branchCountDesc,
                                []()->int{ return level2.branchCount; }, [](int newValue)->void { level2.branchCount = newValue; } ),
            new InputValue<int>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*14, branchCountDeltaMin, branchCountDeltaMax, branchCountDeltaName, branchCountDeltaDesc,
                                []()->int{ return level2.branchCountDelta; }, [](int newValue)->void { level2.branchCountDelta = newValue; } ),
        }),
        // Branch Level 3
        new Menu(3, {
            new InputValue<float>(0, branchLengthMin, branchLengthMax, branchLengthName, branchLengthDesc, []()->float{ return level3.branchLength; },
            [](float newValue)->void { level3.branchLength = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*1, branchLengthDeltaMin, branchLengthDeltaMax, branchLengthDeltaName, branchLengthDeltaDesc,
                                  []()->float{ return level3.branchLengthDelta; }, [](float newValue)->void { level3.branchLengthDelta = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*2, tipMin, tipMax, tipName, tipDesc,
                                  []()->float{ return level3.branchTip; }, [](float newValue)->void { level3.branchTip = newValue; } ),
            new InputValue<float>(deltaSpace*0 + (spacing + INPUT_HEIGHT)*3, tipDeltaMin, tipDeltaMax, tipDeltaName, tipDeltaDesc,
                                  []()->float{ return level3.branchTipDelta; }, [](float newValue)->void { level3.branchTipDelta = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*4, branchSplitAngleMin, branchSplitAngleMax, branchSplitAngleName, branchSplitAngleDesc,
                                  []()->float{ return level3.branchSplitAngle; }, [](float newValue)->void { level3.branchSplitAngle = newValue; } ),
            new InputValue<float>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*5, branchSplitAngleDeltaMin, branchSplitAngleDeltaMax, branchSplitAngleDeltaName, branchSplitAngleDeltaDesc,
                                  []()->float{ return level3.branchSplitAngleDelta; }, [](float newValue)->void { level3.branchSplitAngleDelta = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*6, branchCurvatureMin, branchCurvatureMax, branchCurvatureName, branchCurvatureDesc,
                                  []()->float{ return level3.branchCurvature; }, [](float newValue)->void { level3.branchCurvature = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*7, branchCurvatureDeltaMin, branchCurvatureDeltaMax, branchCurvatureDeltaName, branchCurvatureDeltaDesc,
                                  []()->float{ return level3.branchCurvatureDelta; }, [](float newValue)->void { level3.branchCurvatureDelta = newValue; } ),
            new InputValue<int>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*8, 1, 3, branchSubdivisionsName, branchSubdivisionsDesc,
                                []()->int{ return level3.branchSubdivisions; }, [](int newValue)->void { level3.branchSubdivisions = newValue; } ),
        }),
        // Leaves
        new Menu(4, {
            new InputValue<int>(0, minLeafLevelMin, minLeafLevelMax, minLeafLevelName, minLeafLevelDesc, []()->int{ return minLeafLevel; },
            [](int newValue)->void { minLeafLevel = newValue; } ),
            new InputValue<int>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*1, leafTypeMin, leafTypeMax, leafTypeName, leafTypeDesc, []()->int{ return leafType; },
            [](int newValue)->void { leafType = newValue; } ),
            new InputValue<int>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*2, leafCountMin, leafCountMax, leafCountName, leafCountDesc,
                                  []()->int{ return leafCount; }, [](int newValue)->void { leafCount = newValue; } ),
            new InputValue<int>(deltaSpace*1 + (spacing + INPUT_HEIGHT)*3, leafCountDeltaMin, leafCountDeltaMax, leafCountDeltaName, leafCountDeltaDesc,
                                  []()->int{ return leafCountDelta; }, [](int newValue)->void { leafCountDelta = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*4, leafSizeMin, leafSizeMax, leafSizeName, leafSizeDesc,
                                  []()->float{ return leafSize; }, [](float newValue)->void { leafSize = newValue; } ),
            new InputValue<float>(deltaSpace*2 + (spacing + INPUT_HEIGHT)*5, leafSizeDeltaMin, leafSizeDeltaMax, leafSizeDeltaName, leafSizeDeltaDesc,
                                  []()->float{ return leafSizeDelta; }, [](float newValue)->void { leafSizeDelta = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*6, leafSizeXMin, leafSizeXMax, leafSizeXName, leafSizeXDesc,
                                  []()->float{ return leafSizeX; }, [](float newValue)->void { leafSizeX = newValue; } ),
            new InputValue<float>(deltaSpace*3 + (spacing + INPUT_HEIGHT)*7, leafSizeXDeltaMin, leafSizeXDeltaMax, leafSizeXDeltaName, leafSizeXDeltaDesc,
                                  []()->float{ return leafSizeXDelta; }, [](float newValue)->void { leafSizeXDelta = newValue; } ),
            new InputValue<float>(deltaSpace*5 + (spacing + INPUT_HEIGHT)*8, leafBendMin, leafBendMax, leafBendName, leafBendDesc,
                                  []()->float{ return leafBend; }, [](float newValue)->void { leafBend = newValue; } ),
            new InputValue<float>(deltaSpace*4 + (spacing + INPUT_HEIGHT)*9, leafBendDeltaMin, leafBendDeltaMax, leafBendDeltaName, leafBendDeltaDesc,
                                  []()->float{ return leafBendDelta; }, [](float newValue)->void { leafBendDelta = newValue; } ),
            new InputValue<int>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*10, leafZoomMin, leafZoomMax, leafZoomName, leafZoomDesc,
                                  []()->int{ return leafZoom; }, [](int newValue)->void { leafZoom = newValue; } ),
            new InputValue<float>(deltaSpace*7 + (spacing + INPUT_HEIGHT)*11, leafOffsetMin, leafOffsetMax, leafOffsetName, leafOffsetDesc,
                                  []()->float{ return leafOffset; }, [](float newValue)->void { leafOffset = newValue; } ),
            new InputValue<float>(deltaSpace*6 + (spacing + INPUT_HEIGHT)*12, leafOffsetDeltaMin, leafOffsetDeltaMax, leafOffsetDeltaName, leafOffsetDeltaDesc,
                                  []()->float{ return leafOffsetDelta; }, [](float newValue)->void { leafOffsetDelta = newValue; } ),
        }),
    };

    // Make Sure Files Exist
    for(int i = 0; i < MAX_SESSIONS; i++)
    {
        selectedSession = i+1;
        LoadSession(true);
        selectedSession = 1;
    }
    LoadOptions();
    LoadSession();

    sessionButtons[selectedSession-1]->selected = true;
    for (int i = 0; i < tabs.size(); i++) { if (tabs[i]->id == selectedTab) { tabs[i]->selected = true; break; } }
    for (int i = 0; i < menus.size(); i++) { menus[i]->OnTabChange(selectedTab); }
}
