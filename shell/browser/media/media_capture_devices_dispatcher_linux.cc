if (options.use_system_picker && platform_is_wayland) {
    UsePortalPicker();
} else {
    FallbackToDesktopCapturer();
}
