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

final class OESyncStatusOverlayView: NSView {
    
    private let statusLabel = NSTextField(labelWithString: "")
    private let spinner = NSProgressIndicator()
    
    override init(frame: NSRect) {
        super.init(frame: frame)
        setupView()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupView()
    }
    
    private func setupView() {
        wantsLayer = true
        layer?.cornerRadius = 8
        layer?.backgroundColor = NSColor(white: 0.0, alpha: 0.75).cgColor
        
        statusLabel.textColor = .white
        statusLabel.font = .systemFont(ofSize: 13, weight: .medium)
        statusLabel.translatesAutoresizingMaskIntoConstraints = false
        
        spinner.style = .spinning
        spinner.controlSize = .small
        spinner.translatesAutoresizingMaskIntoConstraints = false
        if #available(macOS 14.0, *) {
            spinner.appearance = NSAppearance(named: .darkAqua)
            spinner.contentFilters = []
        }
        
        addSubview(statusLabel)
        addSubview(spinner)
        
        NSLayoutConstraint.activate([
            spinner.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 12),
            spinner.centerYAnchor.constraint(equalTo: centerYAnchor),
            
            statusLabel.leadingAnchor.constraint(equalTo: spinner.trailingAnchor, constant: 8),
            statusLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -12),
            statusLabel.centerYAnchor.constraint(equalTo: centerYAnchor),
            
            heightAnchor.constraint(equalToConstant: 36)
        ])
        
        alphaValue = 0
    }
    
    func show(status: OESyncStatus, message: String?) {
        statusLabel.stringValue = message ?? ""
        
        if status == .connecting || status == .syncing {
            spinner.isHidden = false
            spinner.startAnimation(nil)
        } else {
            spinner.isHidden = true
            spinner.stopAnimation(nil)
        }
        
        NSAnimationContext.runAnimationGroup { context in
            context.duration = 0.3
            self.animator().alphaValue = 1.0
        }
        
        if status == .success || status == .failed {
            // Auto hide after a few seconds
            DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
                self.hide()
            }
        }
    }
    
    func hide() {
        NSAnimationContext.runAnimationGroup { context in
            context.duration = 0.3
            self.animator().alphaValue = 0.0
        }
    }
}
