// Copyright (c) 2020, OpenEmu Team
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

extension NSNotification.Name {
    static let OELibrarySplitViewResetSidebar = NSNotification.Name("OELibrarySplitViewResetSidebar")
}

final class LibraryGamesViewController: NSSplitViewController {
    
    private static let skipDiscGuideMessageKey = "OESkipDiscGuideMessageKey"
    private lazy var discGuideMessageSystemIDs: [String?] = []
    
    private let sidebarMinWidth: CGFloat = 105
    private let sidebarDefaultWidth: CGFloat = 200
    private let sidebarMaxWidth: CGFloat = 450
    private let collectionViewMinWidth: CGFloat = 495
    
    private var sidebarController: SidebarController!
    private var collectionController: OEGameCollectionViewController!
    private var libraryGradientView: LibraryGradientView?
    
    private var toolbar: LibraryToolbar? {
        view.window?.toolbar as? LibraryToolbar
    }
    
    var database: OELibraryDatabase?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        sidebarController = SidebarController()
        collectionController = OEGameCollectionViewController()
        
        setUpSplitView()
        assignDatabase()
        
        NotificationCenter.default.addObserver(self, selector: #selector(updateCollectionContentsFromSidebar), name: .OESidebarSelectionDidChange, object: nil)
        
        // Liquid Glass Aesthetics
        view.wantsLayer = true
        view.layer?.backgroundColor = NSColor.clear.cgColor
        
        // Retro Gradient Tint
        let gradientView = LibraryGradientView(frame: view.bounds)
        gradientView.autoresizingMask = [.width, .height]
        view.addSubview(gradientView, positioned: .below, relativeTo: nil)
        self.libraryGradientView = gradientView
        print("DEBUG: Dynamic Tint - LibraryGradientView initialized")
    }


    
    override func viewWillAppear() {
        super.viewWillAppear()
        
        setUpSplitViewAutosave()
        updateCollectionContentsFromSidebar()
        
        view.needsDisplay = true
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        
        collectionController.updateBlankSlate()
        
        if #available(macOS 11.0, *) {
            view.window?.titlebarSeparatorStyle = .automatic
        }
    }
    
    override func viewWillDisappear() {
        super.viewWillDisappear()
        
        if #available(macOS 11.0, *) {
            view.window?.titlebarSeparatorStyle = .line
        }
    }
    
    private func assignDatabase() {
        sidebarController.database = database
        collectionController.database = database
    }
    
    // MARK: - Split View
    
    private func setUpSplitView() {
        
        splitView.translatesAutoresizingMaskIntoConstraints = false
        splitView.wantsLayer = true
        splitView.layer?.backgroundColor = NSColor.clear.cgColor
        
        let sidebarItem = NSSplitViewItem(sidebarWithViewController: sidebarController)
        sidebarItem.minimumThickness = sidebarMinWidth
        sidebarItem.maximumThickness = sidebarMaxWidth
        sidebarItem.canCollapse = false
        if #available(macOS 11.0, *) {
            sidebarItem.titlebarSeparatorStyle = .automatic
        }
        addSplitViewItem(sidebarItem)
        
        let collectionItem = NSSplitViewItem(viewController: collectionController)
        collectionItem.minimumThickness = collectionViewMinWidth
        if #available(macOS 11.0, *) {
            collectionItem.titlebarSeparatorStyle = .line
        }
        addSplitViewItem(collectionItem)
    }
    
    private func setUpSplitViewAutosave() {
        
        if splitView.autosaveName != nil && !(splitView.autosaveName == "") {
            return
        }
        
        let autosaveName = "OELibraryGamesSplitView"
        
        if UserDefaults.standard.object(forKey: "NSSplitView Subview Frames \(autosaveName)") == nil {
            splitView.setPosition(sidebarDefaultWidth, ofDividerAt: 0)
        }
        
        splitView.autosaveName = autosaveName
        
        NotificationCenter.default.addObserver(self, selector: #selector(resetSidebar), name: .OELibrarySplitViewResetSidebar, object: nil)
    }
    
    @objc func resetSidebar() {
        splitView.setPosition(sidebarDefaultWidth, ofDividerAt: 0)
    }
    
    // MARK: - Actions
    
    @IBAction func newCollection(_ sender: AnyObject?) {
        sidebarController.newCollection(sender)
    }
    
    @IBAction func selectSystems(_ sender: Any?) {
        sidebarController.selectSystems(sender)
    }
    
    @IBAction func switchToGridView(_ sender: Any?) {
        collectionController.showGridView()
    }
    
    @IBAction func switchToListView(_ sender: Any?) {
        collectionController.showListView()
    }
    
    @IBAction func search(_ sender: NSSearchField?) {
        guard let searchField = sender else { return }
        collectionController.performSearch(searchField.stringValue)
    }
    
    @IBAction func changeGridSize(_ sender: NSSlider?) {
        guard let slider = toolbar?.gridSizeSlider else { return }
        collectionController.zoomGridView(withValue: CGFloat(slider.doubleValue))
    }
    
    @IBAction func decreaseGridSize(_ sender: AnyObject?) {
        guard let slider = toolbar?.gridSizeSlider else { return }
        slider.doubleValue -= sender?.tag == 199 ? 0.25 : 0.5
        collectionController.zoomGridView(withValue: CGFloat(slider.doubleValue))
    }
    
    @IBAction func increaseGridSize(_ sender: AnyObject?) {
        guard let slider = toolbar?.gridSizeSlider else { return }
        slider.doubleValue += sender?.tag == 199 ? 0.25 : 0.5
        collectionController.zoomGridView(withValue: CGFloat(slider.doubleValue))
    }
    
    @objc func updateCollectionContentsFromSidebar() {
        
        let selectedItem = sidebarController.selectedSidebarItem
        collectionController.representedObject = selectedItem as? GameCollectionViewItemProtocol
        
        // Dynamic Background Update
        if let system = selectedItem as? OEDBSystem {
            print("DEBUG: Dynamic Tint - Updating for system: \(system.systemIdentifier)")
            updateBackgroundForSystem(system)
        } else {
            if let item = selectedItem as? SidebarItem {
                print("DEBUG: Dynamic Tint - Selected item is NOT a system: \(item.sidebarName)")
            }
            // Default tint for collections/all games
            libraryGradientView?.updateColors(
                start: NSColor.systemPink.withAlphaComponent(0.15),
                end: NSColor.systemOrange.withAlphaComponent(0.15)
            )
        }
        
        // For empty collections of disc-based games, display an alert to compel the user to read the disc-importing guide.
        // Delay disc alert by 200 ms to allow navigating past disc-based systems with arrow keys in the sidebar without triggering the alert.
        DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(200)) {
            guard self.sidebarController.selectedSidebarItem === selectedItem else { return }
            
            if let system = selectedItem as? OEDBSystem,
               system.plugin?.supportsDiscsWithDescriptorFile ?? false,
               system.games.isEmpty,
               !self.discGuideMessageSystemIDs.contains(system.systemIdentifier),
               !UserDefaults.standard.bool(forKey: Self.skipDiscGuideMessageKey),
               let window = self.view.window {
                
                let alert = NSAlert()
                alert.messageText = NSLocalizedString("Have you read the guide?", comment: "")
                alert.informativeText = NSLocalizedString("Disc-based games have special requirements. Please read the disc importing guide.", comment: "")
                alert.alertStyle = .informational
                alert.addButton(withTitle: NSLocalizedString("View Guide in Browser", comment: ""))
                alert.addButton(withTitle: NSLocalizedString("Dismiss", comment: ""))
                
                alert.beginSheetModal(for: window) { result in
                    if result == .alertFirstButtonReturn {
                        NSWorkspace.shared.open(.userGuideCDBasedGames)
                    }
                }
                
                self.discGuideMessageSystemIDs.append(system.systemIdentifier)
            }
        }
    }
    
    private func updateBackgroundForSystem(_ system: OEDBSystem) {
        let colors: (NSColor, NSColor)
        let opacity: CGFloat = 0.35 // Slightly more vibrant
        let sysID = system.systemIdentifier
        
        print("DEBUG: Dynamic Tint - Applying colors for systemID: \(sysID)")

        switch sysID {
        // --- NINTENDO ---
        case "openemu.system.nes", "openemu.system.fds":
            colors = (NSColor(red: 0.9, green: 0.1, blue: 0.1, alpha: 1.0), .white) // Nintendo Red
        case "openemu.system.snes":
            colors = (NSColor(red: 0.5, green: 0.2, blue: 0.7, alpha: 1.0), NSColor(red: 0.3, green: 0.1, blue: 0.5, alpha: 1.0)) // SNES Purple
        case "openemu.system.n64":
            colors = (NSColor.systemBlue, NSColor.systemRed)
        case "openemu.system.gb", "openemu.system.gbc":
            colors = (NSColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 1.0), .white) // GameBoy Green
        case "openemu.system.gba":
            colors = (.systemIndigo, .systemPink)
        case "openemu.system.nds":
            colors = (.systemTeal, .white)
        case "openemu.system.3ds":
            colors = (.systemRed, .systemBlue)
        case "openemu.system.vb":
            colors = (.black, NSColor(red: 0.8, green: 0.0, blue: 0.0, alpha: 1.0)) // Virtual Boy
        case "openemu.system.gc":
            colors = (.systemPurple, .black)
        case "openemu.system.wii":
            colors = (.white, .systemBlue)

        // --- SEGA ---
        case "openemu.system.genesis", "openemu.system.master", "openemu.system.scd", "openemu.system.sms", "openemu.system.gamegear", "openemu.system.gg", "openemu.system.sg1000", "openemu.system.sg", "openemu.system.32x":
            colors = (NSColor(red: 0.0, green: 0.24, blue: 0.6, alpha: 1.0), .white) // Classic Sega Blue
        case "openemu.system.saturn":
            colors = (.lightGray, .systemBlue)
        case "openemu.system.dreamcast":
            colors = (.systemOrange, .white)

        // --- SONY ---
        case "openemu.system.psx":
            colors = (.white, .systemGray)
        case "openemu.system.psp":
            colors = (.black, .systemBlue)
        case "openemu.system.ps2":
            colors = (.systemBlue, .black)

        // --- ATARI ---
        case "openemu.system.2600", "openemu.system.5200", "openemu.system.7800":
            colors = (.brown, .black)
        case "openemu.system.lynx":
            colors = (.darkGray, .systemOrange)

        // --- OTHERS ---
        case "openemu.system.c64":
            colors = (.systemBlue, .systemPurple)
        case "openemu.system.colecovision":
            colors = (NSColor(red: 0.0, green: 0.0, blue: 0.4, alpha: 1.0), .white)
        case "openemu.system.intellivision":
            colors = (NSColor(red: 0.4, green: 0.2, blue: 0.0, alpha: 1.0), NSColor(red: 1.0, green: 0.8, blue: 0.0, alpha: 1.0))
        case "openemu.system.pce", "openemu.system.tg16", "openemu.system.pcecd", "openemu.system.tgcd":
            colors = (.systemOrange, .black)
        case "openemu.system.neogeo":
            colors = (.systemYellow, .systemRed)
        case "openemu.system.ws", "openemu.system.wsc":
            colors = (.systemPink, .white)

        default:
            print("WARNING: No signature color for systemID: \(sysID)")
            colors = (.systemBlue, .systemIndigo)
        }

        libraryGradientView?.updateColors(
            start: colors.0.withAlphaComponent(opacity),
            end: colors.1.withAlphaComponent(opacity)
        )
    }
    
    @objc func makeNewCollectionWithSelectedGames(_ sender: Any?) {
        assert(Thread.isMainThread, "Only call on main thread!")
        
        sidebarController.newCollection(games: selectedGames)
    }
}

extension LibraryGamesViewController: LibrarySubviewControllerGameSelection {
    
    var selectedGames: [OEDBGame] {
        collectionController.selectedGames
    }
}

// MARK: - Validation

extension LibraryGamesViewController: NSMenuItemValidation {
    
    func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
        let isGridView = collectionController.selectedViewTag == .gridViewTag
        let isBlankSlate = collectionController.shouldShowBlankSlate()
        
        switch menuItem.action {
        case #selector(switchToGridView):
            menuItem.state = isGridView ? .on : .off
            return !isBlankSlate
        case #selector(switchToListView):
            menuItem.state = !isGridView ? .on : .off
            return !isBlankSlate
        default:
            return true
        }
    }
}

final class LibraryGradientView: NSView {
    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        wantsLayer = true
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func makeBackingLayer() -> CALayer {
        let layer = CAGradientLayer()
        // Default Sunset Pink Palette: Pink -> Orange
        layer.colors = [
            NSColor.systemPink.withAlphaComponent(0.15).cgColor,
            NSColor.systemOrange.withAlphaComponent(0.15).cgColor
        ]
        layer.startPoint = CGPoint(x: 0, y: 0)
        layer.endPoint = CGPoint(x: 1, y: 1) // Top-Left to Bottom-Right
        return layer
    }
    
    func updateColors(start: NSColor, end: NSColor) {
        guard let layer = self.layer as? CAGradientLayer else { 
            print("DEBUG: Dynamic Tint - LibraryGradientView updateColors FAILED - Layer is not CAGradientLayer")
            return 
        }
        print("DEBUG: Dynamic Tint - LibraryGradientView updating colors to \(start) and \(end)")
        layer.colors = [start.cgColor, end.cgColor]
        layer.setNeedsDisplay()
    }
}
