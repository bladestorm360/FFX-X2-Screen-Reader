#pragma once
namespace ffx {
// Return the detected encoding in polish; set known only when evidence is conclusive.
using TextEncodingDetector = bool(*)(bool& known);
void setTextEncodingDetector(TextEncodingDetector detector);
void installTextEncoding();
}
