#pragma once
#include <memory>
#include <string>
namespace SessionSketch {struct State;}
class SettingsStateAndEvents;class IMCCStateHook;class GUIElementStore;class TokenSharedRequestProvider;
class QuickMenu {
    class Impl;std::unique_ptr<Impl> p;
public:
    QuickMenu(std::shared_ptr<SettingsStateAndEvents>,std::shared_ptr<IMCCStateHook>,std::shared_ptr<GUIElementStore>,std::shared_ptr<TokenSharedRequestProvider>,const std::string& directory);
    ~QuickMenu();
    void frame(bool conflictingUI,const SessionSketch::State& geometry);
    void settings();
    bool mainMenu(bool open,bool movement,bool conflict,bool camera=false,bool full=false,bool wheelConflict=false); // Updates render heartbeat; true only with a verified filter/route.
    bool mainMenuWheelActive()const;
    bool mainMenuWheelAvailable();
    void cancel();
    void pump(); // Application owner thread only; never install/retire from render or input callbacks.
    bool shutdown(); // Retry until callbacks AND their return stubs are quiescent.
    static bool verifiedSoloCampaign();
    bool verifiedPlayer(unsigned game);
};
