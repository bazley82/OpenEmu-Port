// Copyright (c) 2024, OpenEmu Team
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the OpenEmu Team nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import Cocoa

/// Preferences pane that lets users connect/disconnect their Google Drive account
/// for the Save Sync feature and see the current connection status.
final class PrefCloudSyncController: NSViewController {
    
    // MARK: - UI Elements
    
    private let headerLabel       = NSTextField(labelWithString: "")
    private let descLabel         = NSTextField(wrappingLabelWithString: "")
    private let signInButton      = NSButton()
    private let signOutButton     = NSButton()
    private let statusDot         = NSTextField(labelWithString: "●")
    private let statusLabel       = NSTextField(labelWithString: "")
    private let divider           = NSBox()
    private let lastSyncedLabel   = NSTextField(labelWithString: "")
    private let syncNowButton     = NSButton()
    private let syncInfoLabel     = NSTextField(wrappingLabelWithString: "")
    private let loadingIndicator  = NSProgressIndicator()
    
    private let scrollView        = NSScrollView()
    private let tableView         = NSTableView()
    
    private var cloudFiles: [OESaveSyncManager.DriveFile] = []
    
    private lazy var dateFormatter: DateFormatter = {
        let df = DateFormatter()
        df.dateStyle = .medium
        df.timeStyle = .short
        return df
    }()
    
    private lazy var tableDateFormatter: DateFormatter = {
        let df = DateFormatter()
        df.dateStyle = .short
        df.timeStyle = .short
        return df
    }()
    
    // MARK: - Notification Token
    
    private var syncStatusToken: NSObjectProtocol?
    
    // MARK: - Lifecycle
    
    override func loadView() {
        view = NSView(frame: NSRect(x: 0, y: 0, width: 468, height: 480))
        buildUI()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupTableView()
        updateStatus()
        
        syncStatusToken = NotificationCenter.default.addObserver(
            forName: .OESaveSyncStatusDidChange,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            self?.updateStatus()
        }
    }
    
    deinit {
        if let token = syncStatusToken {
            NotificationCenter.default.removeObserver(token)
        }
    }
    
    // MARK: - TableView Setup
    
    private func setupTableView() {
        tableView.delegate = self
        tableView.dataSource = self
        tableView.headerView = NSTableHeaderView()
        tableView.rowHeight = 20
        tableView.gridStyleMask = .solidHorizontalGridLineMask
        tableView.columnAutoresizingStyle = .uniformColumnAutoresizingStyle
        
        let sysCol = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("System"))
        sysCol.headerCell.stringValue = "System"
        sysCol.width = 60
        tableView.addTableColumn(sysCol)
        
        let fileCol = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("Filename"))
        fileCol.headerCell.stringValue = "Filename"
        fileCol.width = 200
        tableView.addTableColumn(fileCol)
        
        let dateCol = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("Modified"))
        dateCol.headerCell.stringValue = "Modified"
        dateCol.width = 120
        tableView.addTableColumn(dateCol)
        
        scrollView.documentView = tableView
        scrollView.hasVerticalScroller = true
        scrollView.borderType = .bezelBorder
    }
    
    // MARK: - Build UI
    
    private func buildUI() {
        // ── Header ──────────────────────────────────────────────────
        headerLabel.stringValue = "Google Drive Cloud Sync"
        headerLabel.font = .boldSystemFont(ofSize: 15)
        headerLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(headerLabel)
        
        // ── Description ─────────────────────────────────────────────
        descLabel.stringValue = "Sign in with Google to automatically back up your battery saves and save states. When launching a game, OpenEmu will check whether a newer save is available in the cloud and offer to download it."
        descLabel.font = .systemFont(ofSize: 12)
        descLabel.textColor = .secondaryLabelColor
        descLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(descLabel)
        
        // ── Status Row ───────────────────────────────────────────────
        statusDot.font = .systemFont(ofSize: 14)
        statusDot.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(statusDot)
        
        statusLabel.font = .systemFont(ofSize: 13, weight: .medium)
        statusLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(statusLabel)
        
        // ── Sign In Button ───────────────────────────────────────────
        signInButton.title = "Sign In with Google"
        signInButton.bezelStyle = .rounded
        signInButton.controlSize = .large
        signInButton.font = .systemFont(ofSize: 13, weight: .semibold)
        signInButton.target = self
        signInButton.action = #selector(signIn)
        signInButton.translatesAutoresizingMaskIntoConstraints = false
        // Use the Google brand colour as much as AppKit allows.
        signInButton.contentTintColor = NSColor(red: 0.259, green: 0.522, blue: 0.957, alpha: 1)
        view.addSubview(signInButton)
        
        // ── Sign Out Button ──────────────────────────────────────────
        signOutButton.title = "Sign Out"
        signOutButton.bezelStyle = .rounded
        signOutButton.controlSize = .regular
        signOutButton.font = .systemFont(ofSize: 12)
        signOutButton.target = self
        signOutButton.action = #selector(signOut)
        signOutButton.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(signOutButton)
        
        // ── Divider ──────────────────────────────────────────────────
        divider.boxType = .separator
        divider.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(divider)
        
        // ── Loading Indicator ────────────────────────────────────────
        loadingIndicator.style = .spinning
        loadingIndicator.controlSize = .small
        loadingIndicator.isDisplayedWhenStopped = false
        loadingIndicator.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(loadingIndicator)
        
        // ── ScrollView / TableView ───────────────────────────────────
        scrollView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(scrollView)
        
        // ── Sync Now Button ──────────────────────────────────────────
        syncNowButton.title = "Sync Now"
        syncNowButton.bezelStyle = .rounded
        syncNowButton.controlSize = .small
        syncNowButton.font = .systemFont(ofSize: 11)
        syncNowButton.target = self
        syncNowButton.action = #selector(syncNow)
        syncNowButton.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(syncNowButton)
        
        // ── Last Synced Label ────────────────────────────────────────
        lastSyncedLabel.font = .systemFont(ofSize: 11)
        lastSyncedLabel.textColor = .secondaryLabelColor
        lastSyncedLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(lastSyncedLabel)
        
        // ── Sync Info ────────────────────────────────────────────────
        syncInfoLabel.stringValue = "Saves are stored in a hidden App Data folder in your Google Drive."
        syncInfoLabel.font = .systemFont(ofSize: 11)
        syncInfoLabel.textColor = .tertiaryLabelColor
        syncInfoLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(syncInfoLabel)
        
        // ── Layout ───────────────────────────────────────────────────
        NSLayoutConstraint.activate([
            // Header
            headerLabel.topAnchor.constraint(equalTo: view.topAnchor, constant: 32),
            headerLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            headerLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -36),
            
            // Description
            descLabel.topAnchor.constraint(equalTo: headerLabel.bottomAnchor, constant: 10),
            descLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            descLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -36),
            
            // Status dot + label
            statusDot.topAnchor.constraint(equalTo: descLabel.bottomAnchor, constant: 24),
            statusDot.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            
            statusLabel.centerYAnchor.constraint(equalTo: statusDot.centerYAnchor),
            statusLabel.leadingAnchor.constraint(equalTo: statusDot.trailingAnchor, constant: 6),
            
            // Sign In button
            signInButton.topAnchor.constraint(equalTo: statusDot.bottomAnchor, constant: 20),
            signInButton.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            signInButton.widthAnchor.constraint(equalToConstant: 180),
            
            // Sign Out button
            signOutButton.centerYAnchor.constraint(equalTo: signInButton.centerYAnchor),
            signOutButton.leadingAnchor.constraint(equalTo: signInButton.trailingAnchor, constant: 12),
            
            // Divider
            divider.topAnchor.constraint(equalTo: signInButton.bottomAnchor, constant: 24),
            divider.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 20),
            divider.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -20),
            
            // Loading Indicator
            loadingIndicator.centerYAnchor.constraint(equalTo: statusDot.centerYAnchor),
            loadingIndicator.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -36),
            
            // ScrollView (Cloud Files List)
            scrollView.topAnchor.constraint(equalTo: divider.bottomAnchor, constant: 16),
            scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -36),
            scrollView.heightAnchor.constraint(equalToConstant: 180),
            
            // Sync Now
            syncNowButton.topAnchor.constraint(equalTo: scrollView.bottomAnchor, constant: 12),
            syncNowButton.trailingAnchor.constraint(equalTo: scrollView.trailingAnchor),
            
            // Last Synced
            lastSyncedLabel.centerYAnchor.constraint(equalTo: syncNowButton.centerYAnchor),
            lastSyncedLabel.leadingAnchor.constraint(equalTo: scrollView.leadingAnchor),
            
            // Privacy Note
            syncInfoLabel.topAnchor.constraint(equalTo: syncNowButton.bottomAnchor, constant: 12),
            syncInfoLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 36),
            syncInfoLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -36),
        ])
    }
    
    // MARK: - Status Update
    
    private func updateStatus() {
        let isSignedIn = OESaveSyncManager.shared.isSignedIn
        
        if isSignedIn {
            statusDot.textColor    = NSColor(red: 0.2, green: 0.78, blue: 0.35, alpha: 1)  // green
            statusLabel.stringValue = "Connected"
            statusLabel.textColor   = NSColor(red: 0.2, green: 0.78, blue: 0.35, alpha: 1)
            fetchCloudFiles()
        } else {
            statusDot.textColor    = NSColor(red: 0.87, green: 0.20, blue: 0.18, alpha: 1) // red
            statusLabel.stringValue = "Not Connected"
            statusLabel.textColor   = NSColor(red: 0.87, green: 0.20, blue: 0.18, alpha: 1)
            cloudFiles = []
            tableView.reloadData()
        }
        
        signInButton.isHidden  = isSignedIn
        signOutButton.isHidden = !isSignedIn
        
        scrollView.isHidden    = !isSignedIn
        syncNowButton.isHidden = !isSignedIn
        lastSyncedLabel.isHidden = !isSignedIn
        
        if isSignedIn {
            if let date = OESaveSyncManager.shared.lastSyncDate {
                lastSyncedLabel.stringValue = "Last synced: \(dateFormatter.string(from: date))"
            } else {
                lastSyncedLabel.stringValue = "Not synced yet"
            }
        }
    }
    
    private func fetchCloudFiles() {
        guard OESaveSyncManager.shared.isSignedIn else { return }
        
        loadingIndicator.startAnimation(nil)
        
        Task {
            do {
                let files = try await OESaveSyncManager.shared.fetchCloudFileList()
                await MainActor.run {
                    self.cloudFiles = files.sorted { ($0.modifiedTime ?? .distantPast) > ($1.modifiedTime ?? .distantPast) }
                    self.tableView.reloadData()
                    self.loadingIndicator.stopAnimation(nil)
                }
            } catch {
                await MainActor.run {
                    self.loadingIndicator.stopAnimation(nil)
                }
            }
        }
    }
    
    // MARK: - Actions
    
    @objc private func signIn() {
        OESaveSyncManager.shared.signIn()
    }
    
    @objc private func signOut() {
        let alert = NSAlert()
        alert.messageText     = "Sign Out of Google Drive?"
        alert.informativeText = "Your local saves will not be affected. You can sign back in at any time."
        alert.addButton(withTitle: "Sign Out")
        alert.addButton(withTitle: "Cancel")
        alert.alertStyle = .warning
        
        guard alert.runModal() == .alertFirstButtonReturn else { return }
        OESaveSyncManager.shared.signOut()
        updateStatus()
    }
    
    @objc private func syncNow() {
        OESaveSyncManager.shared.performFullSyncCheck()
        fetchCloudFiles()
    }
}

// MARK: - TableView Data Source & Delegate

extension PrefCloudSyncController: NSTableViewDataSource, NSTableViewDelegate {
    
    func numberOfRows(in tableView: NSTableView) -> Int {
        return cloudFiles.count
    }
    
    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        let identifier = NSUserInterfaceItemIdentifier("CloudFileCell")
        var view = tableView.makeView(withIdentifier: identifier, owner: self) as? NSTextField
        
        if view == nil {
            view = NSTextField(labelWithString: "")
            view?.identifier = identifier
            view?.font = .systemFont(ofSize: 11)
        }
        
        let file = cloudFiles[row]
        let name = file.name ?? "Unknown"
        
        switch tableColumn?.identifier.rawValue {
        case "System":
            // Attempt to extract system from path: "openemu.system.gba/Game.sav"
            let parts = name.components(separatedBy: CharacterSet(charactersIn: "/\\"))
            if let firstPart = parts.first {
                let sys = String(firstPart).replacingOccurrences(of: "openemu.system.", with: "").uppercased()
                view?.stringValue = sys
            } else {
                view?.stringValue = "???"
            }
            
        case "Filename":
            let parts = name.components(separatedBy: CharacterSet(charactersIn: "/\\"))
            if let lastPart = parts.last {
                view?.stringValue = String(lastPart)
            } else {
                view?.stringValue = name
            }
            
        case "Modified":
            if let date = file.modifiedTime {
                view?.stringValue = tableDateFormatter.string(from: date)
            } else {
                view?.stringValue = "-"
            }
            
        default:
            break
        }
        
        return view
    }
}

// MARK: - PreferencePane

extension PrefCloudSyncController: PreferencePane {
    
    var icon: NSImage? {
        // Use the built-in iCloud/cloud SF Symbol (available macOS 11+), fallback to nil.
        if #available(macOS 11.0, *) {
            return NSImage(systemSymbolName: "icloud.and.arrow.up", accessibilityDescription: "Cloud Sync")
        }
        return NSImage(named: NSImage.networkName)
    }
    
    var panelTitle: String { "Cloud Sync" }
    
    var viewSize: NSSize { NSSize(width: 468, height: 480) }
}
