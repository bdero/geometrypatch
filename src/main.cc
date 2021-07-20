// GeometryWars Patcher
// https://github.com/bdero/geometrypatch

#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <vector>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Text_Display.H>

#include "sha1/sha1.h"

const char* EXPECTED_HASH = "87eaf473003a69cbbbad558be14f0bbc419f52f9";

struct Patch {
  std::vector<char> signature;
  std::vector<char> patch;
  std::optional<size_t> location;
};

std::map<std::string, Patch> PATCHES{
    {"High core crash",
     {{' ', ' ', 'A', 'f', 'f', 'i', 'n', 'i', 't', 'y', 'M',
       'a', 's', 'k', ' ', '=', ' ', '%', 'd', ';'},  // "  AffinityMask = %d;"
      {'\0'}}}};

struct {
  Fl_Window* window;
  Fl_Input* directory_input;
  Fl_Button* file_browse_button;
  Fl_Text_Display* validation_display;
  Fl_Text_Buffer* validation_buffer;
  Fl_Text_Buffer* validation_style_buffer;
  Fl_Button* patch_button;
} w;

void AppendValidationLine(const std::string& text, char style) {
  w.validation_buffer->append(text.c_str());
  w.validation_buffer->append("\n");

  auto length = text.length() + 1;
  auto* style_text = new char[length + 1];
  for (int i = 0; i < length; i++) {
    style_text[i] = style;
  }
  style_text[length] = '\0';
  w.validation_style_buffer->append(style_text);
  delete[] style_text;
}

/// Assigns the first location in the stream which matches the patch location,
/// otherwise assigns `std::nullopt`.
void FindSignature(std::istream& stream, Patch& patch) {
  stream.clear();
  stream.seekg(0, std::ios::beg);

  size_t sig_length = patch.signature.size();
  auto sig_buffer = std::vector<char>(sig_length);
  stream.read(sig_buffer.data(), static_cast<std::streamsize>(sig_length));

  size_t stream_offset = 0;
  while (stream) {
    bool match = true;
    for (size_t i = 0; i < sig_length; i++) {
      size_t c = (stream_offset + i) % sig_length;
      if (sig_buffer[c] != patch.signature[i]) {
        match = false;
        break;
      }
    }
    if (match) {
      patch.location = stream_offset;
      return;
    }

    stream.read(sig_buffer.data() + (stream_offset % sig_length), 1);
    stream_offset++;
  }

  patch.location = std::nullopt;
}

void ValidateDirectory(const std::filesystem::path& path) {
  w.validation_buffer->text("");
  w.validation_style_buffer->text("");

  std::ifstream file(path / "testapp.exe", std::ios::binary);
  if (file.fail()) {
    AppendValidationLine(
        "Failed to find the unpacked GeometryWars binary (testapp.exe)", 'C');

    w.patch_button->deactivate();
    return;
  }
  AppendValidationLine("Found the unpacked GeometryWars binary (testapp.exe).",
                       'B');

  SHA1 digest;
  digest.update(file);
  auto result_hash = digest.final();
  if (strcmp(result_hash.c_str(), EXPECTED_HASH) != 0) {
    AppendValidationLine(
        std::format(
            "Warning: SHA1 mismatch!\n     Expected: {}\n     Computed: {}",
            EXPECTED_HASH, result_hash),
        'C');
  } else {
    AppendValidationLine(std::format("SHA1 match! {}", result_hash), 'B');
  }

  Patch& patch = PATCHES["High core crash"];
  FindSignature(file, patch);
  if (!patch.location.has_value()) {
    AppendValidationLine(
        "High core crash signature not found. File isn't patchable.", 'C');
  } else {
    AppendValidationLine(std::format("High core crash signature found at "
                                     "address `{:X}`. File is patchable.",
                                     patch.location.value()),
                         'B');
  }

  file.close();

  if (patch.location.has_value()) {
    w.patch_button->activate();
  } else {
    w.patch_button->deactivate();
  }
}

void PatchGame(const std::filesystem::path& path) {
  std::ifstream original_file(path / "testapp.exe",
                              std::ios::binary | std::ios::ate);
  std::streamsize size = original_file.tellg();
  original_file.seekg(0, std::ios::beg);

  std::vector<char> patched_buffer(size);
  if (!original_file.read(patched_buffer.data(), size)) {
    AppendValidationLine(
        "Failed to load `testapp.exe` into memory. Patching failed.", 'C');
    return;
  }
  original_file.close();

  for (const auto& [key, value] : PATCHES) {
    if (!value.location.has_value()) {
      continue;
    }
    std::memcpy(patched_buffer.data() + value.location.value(),
                value.patch.data(), value.patch.size());
    AppendValidationLine(std::format("Applied patch: {}", key), 'B');
  }

  auto backup_path = path / "GeometryWars.exe.original";
  auto exe_path = path / "GeometryWars.exe";
  if (!std::filesystem::exists(backup_path)) {
    // If There is no backup, make one.
    std::filesystem::rename(exe_path, backup_path);
    AppendValidationLine(
        "Backed up `GeometryWars.exe` as `GeometryWars.exe.original`.", 'A');
  } else {
    // If there's already a backup, assume the original is patched and delete
    // it.
    std::filesystem::remove(exe_path);
    AppendValidationLine("`GeometryWars.exe.original` backup already exists.",
                         'A');
  }

  std::ofstream new_file(exe_path, std::ios::binary);
  new_file.write(patched_buffer.data(),
                 static_cast<std::streamsize>(patched_buffer.size()));
  new_file.close();

  AppendValidationLine("Patched file saved as `GeometryWars.exe`!", 'B');
}

void BrowseForFile(Fl_Widget* _) {
  Fl_Native_File_Chooser file_chooser;
  file_chooser.title("GeometryWars game directory");
  file_chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
  file_chooser.directory(w.directory_input->value());

  if (file_chooser.show() == 0) {
    w.directory_input->value(file_chooser.filename());
    ValidateDirectory(file_chooser.filename());
  }
}

int main(int argc, char** argv) {
  w.window = new Fl_Window(
      650, 210, "GeometryWars Patcher - github.com/bdero/geometrypatch");
  w.window->icon()

  w.directory_input = new Fl_Input(10, 10, w.window->w() - 100 - 30, 30);
  w.directory_input->value(
      "C:/Program Files (x86)/Steam/steamapps/common/Geometry Wars");
  w.directory_input->when(FL_WHEN_CHANGED);
  w.directory_input->callback([](Fl_Widget* input) {
    ValidateDirectory(reinterpret_cast<Fl_Input*>(input)->value());
  });

  w.file_browse_button = new Fl_Button(
      w.directory_input->x() + w.directory_input->w() + 10,
      w.directory_input->y(), w.window->w() - w.directory_input->w() - 30,
      w.directory_input->h(), "@fileopen  Browse");
  w.file_browse_button->callback(BrowseForFile);

  w.validation_display = new Fl_Text_Display(10, w.directory_input->h() + 20,
                                             w.window->w() - 20, 110);
  w.validation_buffer = new Fl_Text_Buffer();
  w.validation_style_buffer = new Fl_Text_Buffer();
  w.validation_display->buffer(w.validation_buffer);

  w.patch_button =
      new Fl_Button(w.window->w() / 2 - 75,
                    w.directory_input->h() + w.validation_display->h() + 30,
                    150, 30, "@filesaveas  Patch");
  w.patch_button->deactivate();
  w.patch_button->callback(
      [](Fl_Widget* _) { PatchGame(w.directory_input->value()); });

  w.window->end();
  w.window->show(argc, argv);

  static Fl_Text_Display::Style_Table_Entry styletable[] = {
      {FL_BLACK, FL_COURIER, FL_NORMAL_SIZE},            // A - Info
      {FL_DARK_GREEN, FL_COURIER_BOLD, FL_NORMAL_SIZE},  // B - Success
      {FL_DARK_RED, FL_COURIER_BOLD, FL_NORMAL_SIZE},    // C - Failure
  };
  w.validation_display->highlight_data(
      w.validation_style_buffer, styletable,
      sizeof(styletable) / sizeof(styletable[0]), 'A', nullptr, nullptr);

  ValidateDirectory(w.directory_input->value());

  return Fl::run();
}
