package org.openemu.android

import android.content.res.Configuration
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.lifecycleScope
import androidx.window.layout.FoldingFeature
import androidx.window.layout.WindowInfoTracker
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private var isFlexMode by mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Monitor folding state specifically for Flex Mode
        lifecycleScope.launch {
            WindowInfoTracker.getOrCreate(this@MainActivity)
                .windowLayoutInfo(this@MainActivity)
                .collect { newLayoutInfo ->
                    val foldingFeature = newLayoutInfo.displayFeatures
                        .filterIsInstance<FoldingFeature>()
                        .firstOrNull()
                    
                    // Flex mode is typically when the device is half-opened
                    isFlexMode = foldingFeature?.state == FoldingFeature.State.HALF_OPENED
                }
        }

        setContent {
            OpenEmuTheme {
                val configuration = LocalConfiguration.current
                val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
                
                Surface(modifier = Modifier.fillMaxSize(), color = Color.Black) {
                    ResponsiveEmulatorLayout(isLandscape, isFlexMode)
                }
            }
        }
    }

    @Composable
    fun ResponsiveEmulatorLayout(isLandscape: Boolean, isFlexMode: Boolean) {
        if (isFlexMode && !isLandscape) {
            // Foldable Flex Mode (Portrait-ish)
            Column(modifier = Modifier.fillMaxSize()) {
                Box(modifier = Modifier.weight(1f).fillMaxWidth()) {
                    EmulatorVideoSurface()
                }
                Box(modifier = Modifier.weight(1f).fillMaxWidth().background(Color(0xFF1A1A1A))) {
                    OnScreenController(transparent = false)
                }
            }
        } else if (isLandscape) {
            // Universal Landscape Mode (Full screen with Overlay)
            Box(modifier = Modifier.fillMaxSize()) {
                EmulatorVideoSurface()
                // Controller Overlay
                Box(modifier = Modifier.fillMaxSize().padding(16.dp)) {
                    OnScreenController(transparent = true)
                }
            }
        } else {
            // Universal Portrait Mode (Stacked)
            Column(modifier = Modifier.fillMaxSize()) {
                // Video at top, keeping aspect ratio (e.g., 4:3 for GBA implies we might need a fixed height or weight)
                Box(modifier = Modifier.weight(0.4f).fillMaxWidth()) {
                    EmulatorVideoSurface()
                }
                // Controls at bottom
                Box(modifier = Modifier.weight(0.6f).fillMaxWidth().background(Color(0xFF1A1A1A))) {
                    OnScreenController(transparent = false)
                }
            }
        }
    }

    @Composable
    fun EmulatorVideoSurface() {
        AndroidView(
            factory = { context ->
                SurfaceView(context).apply {
                    holder.addCallback(object : SurfaceHolder.Callback {
                        override fun surfaceCreated(holder: SurfaceHolder) {
                            nativeSetSurface(holder.surface)
                        }
                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, w: Int, h: Int) {
                            nativeSetSize(w, h)
                        }
                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            nativeSetSurface(null)
                        }
                    })
                }
            },
            modifier = Modifier.fillMaxSize()
        )
    }

    @Composable
    fun OnScreenController(transparent: Boolean) {
        val alpha = if (transparent) 0.5f else 1.0f
        val color = if (transparent) Color.White.copy(alpha = 0.3f) else MaterialTheme.colorScheme.primary
        
        // Simplified controller layout
        Box(modifier = Modifier.fillMaxSize()) {
            // D-Pad (Left side)
            Column(modifier = Modifier.align(Alignment.CenterStart).padding(start = 24.dp)) {
                Button(onClick = { sendInput("UP") }, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("U") }
                Row {
                    Button(onClick = { sendInput("LEFT") }, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("L") }
                    Spacer(modifier = Modifier.width(8.dp))
                    Button(onClick = { sendInput("RIGHT") }, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("R") }
                }
                Button(onClick = { sendInput("DOWN") }, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("D") }
            }

            // Action Buttons (Right side)
            Row(modifier = Modifier.align(Alignment.CenterEnd).padding(end = 24.dp)) {
                Button(onClick = { sendInput("B") }, shape = androidx.compose.foundation.shape.CircleShape, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("B") }
                Spacer(modifier = Modifier.width(16.dp))
                Button(onClick = { sendInput("A") }, shape = androidx.compose.foundation.shape.CircleShape, colors = ButtonDefaults.buttonColors(containerColor = color)) { Text("A") }
            }
        }
    }

    private fun sendInput(button: String) {
        nativeSendInput(button)
    }

    private external fun nativeSendInput(button: String)
    private external fun nativeLoadROM(path: String)
    private external fun nativeSetSurface(surface: Any?)
    private external fun nativeSetSize(width: Int, height: Int)
    private external fun stringFromJNI(): String

    companion object {
        init {
            System.loadLibrary("mgba")
        }
    }
}

@Composable
fun OpenEmuTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Color(0xFF6200EE),
            background = Color.Black,
            surface = Color(0xFF121212)
        ),
        content = content
    )
}
