#pragma once

// QSettings key names shared between SettingsDialog (which writes them) and
// anything else that needs to read the saved transceiver configuration
// (MainWindow, for keying PTT and tuning the JTTY encoder/decoder).
namespace SettingsKeys {
constexpr auto callsign = "callsign";
constexpr auto appendCallsign = "appendCallsign";
constexpr auto capitalizeText = "capitalizeText";
constexpr auto transceiverGroup = "Transceiver";
constexpr auto audioInputDeviceId = "audioInputDeviceId";
constexpr auto audioOutputDeviceId = "audioOutputDeviceId";
constexpr auto rigModel = "rigModel";
constexpr auto rigPort = "rigPort";
constexpr auto rigBaudRate = "rigBaudRate";
} // namespace SettingsKeys
