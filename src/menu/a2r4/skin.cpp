// Look of the A2 Racer IV menu: picture names that Menu.exe sets in code,
// background pictures, fonts.
#include <map>
#include <string>

#include "a2r4/game.h"

namespace a2 {

// Prefixes of the button pictures in MenuRcs ("<prefix>_up.tga", _do, _fo,
// _hi, _di). TData.InitDefs 0x480828 and the FormCreate of each form call
// ReadBtnDefGraphics 0x481698 with these.
static const std::map<std::string, std::string>& DefPrefixes() {
  static const std::map<std::string, std::string> m = {
      {"TData.gdfBtnPreviousItem", "back2"}, {"TData.gdfBtnNextItem", "next2"},
      {"TData.gdfBtnClose", "end"},          {"TData.gdfBtnHelp", "helpm"},
      {"TData.gdfBtnOkDlg", "okay"},         {"TData.gdfBtnOkSingleDlg", "okas"},
      {"TData.gdfBtnNext", "nextm"},         {"TData.gdfBtnNextSingle", "nexts"},
      {"TData.gdfBtnPrevious", "backm"},     {"TData.gdfBtnCancelDlg", "cance"},
      {"TData.gdfBtnCloseDlg", "cross"},     {"TData.gdfBtnYesDlg", "ryes"},
      {"TData.gdfBtnNoDlg", "lno"},          {"TData.gdfRadioBtn", "radio"},
      {"TData.gdfCheckBox", "check"},
      // TfrmMainScreen.FormCreate 0x49db5c
      {"TfrmMainScreen.gdfBtnTour", "tour"}, {"TfrmMainScreen.gdfBtnQR", "quick"},
      {"TfrmMainScreen.gdfBtnInternet", "inter"}, {"TfrmMainScreen.gdfBtnOptions", "optio"},
      {"TfrmMainScreen.gdfBtnPlayer", "playe"},
      {"TfrmCurrentChar.gdfBtnChoose", "newch"},  // 0x49b704
      {"TfrmDifficulty.gdfBtnEasy", "easy"},      // 0x49d304
      {"TfrmDifficulty.gdfBtnMedium", "norma"},
      {"TfrmDifficulty.gdfBtnHard", "hard"},
      {"TfrmGarage.gdfBtnPrevious3", "back3"},    // 0x498d8c
      {"TfrmGarage.gdfBtnNext3", "next3"},
      {"TfrmGarage.gdfBtnBuyTU", "buy"},
      {"TfrmKeySettings.gdfBtnDefault", "defau"}, // 0x4921c4
      {"TfrmKeySettings.gdfBtnKey", "key"},
      {"TfrmOptions.gdfBtnAdvVideo", "advan"},    // 0x4967fc
      {"TfrmOptions.gdfBtnCtrlSettings", "setti"},
      {"TfrmPlayer.gdfBtnDeletePlayer", "delet"}, // 0x494658
      {"TfrmPlayer.defEmpty", "butp1"},
      {"TfrmCloseDlg.gdfBigRadioBtnMain", "radhm"},  // 0x49e0ac
      {"TfrmCloseDlg.gdfRadioBtnExit", "radex"},
  };
  return m;
}

void RegisterSkin(davi::Skin& skin) {
  skin.data_module = "TData";
  skin.default_font = 0;
  skin.raw_font_index = true;  // MenuCaps "Font=" is a Skin::fonts index as is
  skin.tgdi_dfm_caption = false;  // DFM captions are component names; [DEFAULT] Caption= is empty
  // TGDIButtonDefinition pictures: "<prefix>_<up|do|fo|hi|di>.tga".
  skin.def_link = [](const std::string& key, int state) {
    static const char* suffix[] = {"up", "do", "fo", "hi", "di"};
    auto& m = DefPrefixes();
    auto it = m.find(key);
    if (it == m.end() || state < 0 || state > 4) return std::string();
    return it->second + "_" + suffix[state] + ".tga";
  };
  // TGameMenuScreen.FormCreate 0x49e394 / TfrmCoolDialog: the picture named
  // copy(ClassName, 5, 8) from MenuRcs ("currentc" if there is none).
  skin.background_name = [&skin](const std::string& cls) -> std::string {
    if (cls.size() <= 4) return "";
    std::string n = davi::Lower(cls.substr(4, 8));
    if (skin.assets && (skin.assets->Has(n + ".bmp") || skin.assets->Has(n + ".tga"))) return n;
    for (std::string p = cls; !p.empty(); p = skin.exe->ParentClass(p))
      if (p == "TGameMenuScreen") return "currentc";
    return "";
  };
}

}  // namespace a2
