// OEGoogleDriveSecrets.template.swift
// ─────────────────────────────────────────────────────────────────────────────
// SETUP INSTRUCTIONS
// ─────────────────────────────────────────────────────────────────────────────
// 1. Copy this file and rename it:
//      OEGoogleDriveSecrets.template.swift  →  OEGoogleDriveSecrets.swift
//
// 2. Fill in your credentials from the Google Cloud Console:
//    https://console.cloud.google.com/ → APIs & Services → Credentials
//    (OAuth 2.0 Client ID, type: macOS Desktop)
//
// 3. OEGoogleDriveSecrets.swift is gitignored — never commit it.
// ─────────────────────────────────────────────────────────────────────────────

import Foundation

extension OEGoogleDriveConfig {
    /// Your real Google OAuth Client ID.
    static let _clientID     = "YOUR_CLIENT_ID.apps.googleusercontent.com"
    /// Your real Google OAuth Client Secret.
    static let _clientSecret = "YOUR_CLIENT_SECRET"
}
