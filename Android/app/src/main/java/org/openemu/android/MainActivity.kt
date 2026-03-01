package org.openemu.android

import android.content.res.Configuration
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.compose.runtime.mutableFloatStateOf
import androidx.lifecycle.lifecycleScope
import androidx.window.layout.FoldingFeature
import androidx.window.layout.WindowInfoTracker
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Settings
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.draw.blur
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.asComposeRenderEffect
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.documentfile.provider.DocumentFile
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import android.content.Intent
import android.content.SharedPreferences
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream

// Design Tokens (macOS Parity)
// macOS Palette (Beta 6)
val macOS_Sidebar_Dark = Color(0xFF282828)
val macOS_Library_Dark = Color(0xFF1A1A1A)
val macOS_Sidebar_Light = Color(0xFFEBEBEB)
val macOS_Library_Light = Color(0xFFFFFFFF)
val AppleBlue = Color(0xFF007AFF)
val FrostedGlass = Color.White.copy(alpha = 0.12f)
val VibrantGlass = Color.White.copy(alpha = 0.08f)

/**
 * Returns a Modifier that blurs ONLY the layer it is applied to.
 * Use this on a background Box; place content in a SEPARATE sibling Box above it.
 * This prevents children (text/icons) from being blurred.
 */
@Composable
fun Modifier.liquidGlass(
    blurX: Float = 30f,
    blurY: Float = 30f
): Modifier {
    return if (android.os.Build.VERSION.SDK_INT >= 31) {
        this.graphicsLayer {
            renderEffect = android.graphics.RenderEffect.createBlurEffect(
                blurX, blurY, android.graphics.Shader.TileMode.DECAL
            ).asComposeRenderEffect()
        }
    } else {
        this.blur(20.dp)
    }
}

@OptIn(ExperimentalMaterial3Api::class)
class MainActivity : ComponentActivity() {

    companion object {
        init {
            try {
                System.loadLibrary("libretro_bridge")
            } catch (e: UnsatisfiedLinkError) {
                Log.e("OpenEmuCore", "Failed to load libretro_bridge", e)
            }
        }
        private const val PREFS_NAME   = "prefs_openemu"
        private const val KEY_ROOT_URI = "root_folder_uri"
    }

    private lateinit var prefs: SharedPreferences

    private var isFlexMode by mutableStateOf(false)
    private var isSettingsOpen by mutableStateOf(false)
    private var controllerOpacity by mutableFloatStateOf(0.7f)
    private var rootFolderUri by mutableStateOf<Uri?>(null)
    // Beta 9: store full DocumentFile objects so onClick can access the URI
    private var scannedGames by mutableStateOf<List<DocumentFile>>(emptyList())
    private var selectedSystem by mutableStateOf("Game Boy Advance")
    private var selectedCore by mutableStateOf("mGBA")
    private var psxBiosPath by mutableStateOf<String?>(null)
    private var isPaused by mutableStateOf(false)
    private var isGameRunning by mutableStateOf(false)

    // Beta 11: Synchronization state to delay JNI boot until surface is ready
    private var pendingRomPath by mutableStateOf<String?>(null)
    private var pendingCoreName by mutableStateOf<String?>(null)
    private var pendingLibretroSo by mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)

        // ── Auto-rescan saved folder on launch (SAF Amnesia Fix) ────────────
        prefs.getString(KEY_ROOT_URI, null)?.let { uriStr ->
            val savedUri = Uri.parse(uriStr)
            rootFolderUri = savedUri
            lifecycleScope.launch(Dispatchers.IO) {
                scanFolder(savedUri)
            }
        }

        // Monitor folding state specifically for Flex Mode
        lifecycleScope.launch {
            WindowInfoTracker.getOrCreate(this@MainActivity)
                .windowLayoutInfo(this@MainActivity)
                .collect { newLayoutInfo ->
                    val foldingFeature = newLayoutInfo.displayFeatures
                        .filterIsInstance<FoldingFeature>()
                        .firstOrNull()
                    isFlexMode = foldingFeature?.state == FoldingFeature.State.HALF_OPENED
                }
        }
        
        // Initialize Native Logger (Beta 14)
        getExternalFilesDir(null)?.let {
            nativeInitLogger(it.absolutePath)
        }

        setContent {
            OpenEmuTheme {
                val configuration = LocalConfiguration.current
                val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
                val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
                val scope = rememberCoroutineScope()
                
                // SAF ROM Picker Launcher
                val romPickerLauncher = rememberLauncherForActivityResult(
                    contract = ActivityResultContracts.OpenDocument(),
                    onResult = { uri ->
                        uri?.let { handleRomSelection(it) }
                    }
                )

                // SAF Folder Picker Launcher (Beta 9: persist URI + take persistable permission)
                val folderPickerLauncher = rememberLauncherForActivityResult(
                    contract = ActivityResultContracts.OpenDocumentTree(),
                    onResult = { uri ->
                        uri?.let {
                            // Persist read access across reboots
                            contentResolver.takePersistableUriPermission(
                                it, Intent.FLAG_GRANT_READ_URI_PERMISSION
                            )
                            // Save to SharedPreferences for auto-rescan on next launch
                            prefs.edit().putString(KEY_ROOT_URI, it.toString()).apply()
                            rootFolderUri = it
                            lifecycleScope.launch(Dispatchers.IO) { scanFolder(it) }
                        }
                    }
                )

                // Google Sign-In Launcher
                val googleSignInLauncher = rememberLauncherForActivityResult(
                    contract = ActivityResultContracts.StartActivityForResult(),
                    onResult = { result ->
                        val task = GoogleSignIn.getSignedInAccountFromIntent(result.data)
                        handleSignInResult(task)
                    }
                )

                ModalNavigationDrawer(
                    drawerState = drawerState,
                    drawerContent = {
                        ModalDrawerSheet(
                            drawerContainerColor = macOS_Sidebar_Dark.copy(alpha = 0.95f),
                            drawerShape = RoundedCornerShape(0.dp),
                        ) {
                            // ── Layered Liquid Glass header ─────────────────
                            Box(Modifier.fillMaxWidth().height(48.dp)) {
                                Box(Modifier.matchParentSize().liquidGlass(20f, 20f)
                                    .background(macOS_Sidebar_Dark.copy(alpha = 0.6f)))
                                Text("LIBRARY",
                                    Modifier.align(Alignment.BottomStart).padding(start = 16.dp, bottom = 8.dp),
                                    color = Color.Gray.copy(alpha = 0.6f), fontSize = 11.sp,
                                    fontWeight = FontWeight.Bold, letterSpacing = 1.sp)
                            }

                            // ── Dynamic system list ──────────────────────────
                            val scanned = scannedGames
                            val detected = RomSystemIdentifier.getAllSupportedExtensions()
                                .mapNotNull { ext ->
                                    val matching = scanned.filter { it.name?.endsWith(".$ext", ignoreCase = true) == true }
                                    if (matching.isEmpty()) return@mapNotNull null
                                    RomSystemIdentifier.identify("file.$ext")
                                        ?.let { Triple(it.systemName, ext, matching.size) }
                                }.distinctBy { it.first }

                            val displayList: List<Triple<String, String, Int>> =
                                if (detected.isEmpty()) listOf(
                                    Triple("Game Boy", "gb", 0),
                                    Triple("NES", "nes", 0),
                                    Triple("Nintendo 64", "n64", 0)
                                ) else detected

                            displayList.forEach { (sysName, _, count) ->
                                val dotColor = when (sysName) {
                                    "Game Boy", "Game Boy Color" -> Color(0xFF8BBE1B)
                                    "NES" -> Color(0xFFE60012)
                                    "Nintendo 64" -> Color(0xFF3BA3DC)
                                    "Super Nintendo" -> Color(0xFF51268F)
                                    "Sony PlayStation" -> Color(0xFF00ADB5)
                                    else -> AppleBlue
                                }
                                NavigationDrawerItem(
                                    label = { Text(sysName, color = Color.White) },
                                    badge = if (count > 0) {{ Text("$count", color = Color.Gray, fontSize = 11.sp) }} else null,
                                    selected = selectedSystem == sysName,
                                    onClick = {
                                        selectedSystem = sysName
                                        scope.launch { drawerState.close() }
                                    },
                                    icon = { Box(Modifier.size(10.dp).background(dotColor, CircleShape)) },
                                    colors = NavigationDrawerItemDefaults.colors(
                                        selectedContainerColor = Color.White.copy(alpha = 0.12f),
                                        unselectedContainerColor = Color.Transparent
                                    )
                                )
                            }

                            Divider(Modifier.padding(horizontal = 16.dp, vertical = 8.dp), color = Color.DarkGray)
                            NavigationDrawerItem(
                                label = { Text("Settings", color = Color.White) }, selected = false,
                                onClick = { isSettingsOpen = true; scope.launch { drawerState.close() } },
                                icon = { Icon(Icons.Default.Settings, null, tint = Color.Gray) },
                                colors = NavigationDrawerItemDefaults.colors(unselectedContainerColor = Color.Transparent)
                            )
                            Divider(Modifier.padding(horizontal = 16.dp, vertical = 8.dp), color = Color.DarkGray)
                            NavigationDrawerItem(
                                label = { Text("Connect Cloud", color = Color.White) }, selected = false,
                                onClick = {
                                    val gso = GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                                        .requestEmail()
                                        .requestScopes(com.google.android.gms.common.api.Scope(
                                            com.google.api.services.drive.DriveScopes.DRIVE_APPDATA))
                                        .build()
                                    val client = GoogleSignIn.getClient(this@MainActivity, gso)
                                    googleSignInLauncher.launch(client.signInIntent)
                                },
                                colors = NavigationDrawerItemDefaults.colors(unselectedContainerColor = Color.Transparent)
                            )
                        }
                    }
                ) {
                    Scaffold(
                        topBar = { /* ... */ }
                    ) { padding ->
                        Surface(modifier = Modifier.fillMaxSize().padding(padding), color = macOS_Library_Dark) {
                            if (isSettingsOpen) {
                                SettingsScreen(onClose = { isSettingsOpen = false }, onPickFolder = { folderPickerLauncher.launch(null) })
                            } else if (isGameRunning) {
                                Box(Modifier.fillMaxSize()) {
                                    ResponsiveEmulatorLayout(isLandscape, isFlexMode)
                                    
                                    // Pause Menu Overlay
                                    if (isPaused) {
                                        PauseOverlay(onResume = { isPaused = false }, onQuit = { isGameRunning = false; isPaused = false })
                                    }
                                }
                            } else {
                                GameLibraryGrid()
                            }
                        }
                    }
                }
            }
        }
    }


    @Composable
    fun GameLibraryGrid() {
        // Filter DocumentFile list to those matching the selected system
        val filteredGames = remember(scannedGames, selectedSystem) {
            val systemExtensions = RomSystemIdentifier.getAllSupportedExtensions()
                .filter { ext ->
                    val info = RomSystemIdentifier.identify("file.$ext")
                    info?.systemName == selectedSystem
                }
            if (systemExtensions.isEmpty()) {
                scannedGames
            } else {
                scannedGames.filter { doc ->
                    systemExtensions.any { ext -> doc.name?.endsWith(".$ext", ignoreCase = true) == true }
                }
            }
        }

        Column(Modifier.fillMaxSize().padding(top = 8.dp)) {
            Row(
                Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 12.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(selectedSystem,  color = Color.White, fontSize = 22.sp, fontWeight = FontWeight.ExtraBold)
                Text("${filteredGames.size} games", color = Color.Gray, fontSize = 13.sp)
            }

            if (filteredGames.isEmpty()) {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Icon(Icons.Default.Add, null, tint = Color.Gray, modifier = Modifier.size(48.dp))
                        Spacer(Modifier.height(12.dp))
                        Text("No games found for $selectedSystem", color = Color.Gray)
                        Text("Tap \"Scan Folder\" in Settings to import ROMs",
                            color = Color.DarkGray, fontSize = 12.sp)
                    }
                }
            } else {
                LazyVerticalGrid(
                    columns = GridCells.Adaptive(minSize = 120.dp),
                    contentPadding = PaddingValues(16.dp),
                    horizontalArrangement = Arrangement.spacedBy(16.dp),
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    items(filteredGames) { doc ->
                        GameCard(doc)
                    }
                }
            }
        }
    }

    @Composable
    fun GameCard(doc: DocumentFile) {
        val name = doc.name ?: "Unknown"
        Column(
            modifier = Modifier
                .width(120.dp)
                .clickable {
                    // Beta 9: onClick → ROM Cache Copier → JNI boot → UI transition
                    doc.uri.let { uri -> handleRomSelection(uri) }
                },
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Box(
                modifier = Modifier
                    .size(130.dp, 175.dp)
                    .clip(RoundedCornerShape(12.dp))
                    .background(Brush.verticalGradient(listOf(Color(0xFF2C2C2E), Color(0xFF1C1C1E))))
                    .padding(1.dp),
                contentAlignment = Alignment.Center
            ) {
                Box(Modifier.fillMaxSize().clip(RoundedCornerShape(11.dp)).background(Color.Black)) {
                    Text(
                        name.take(1).uppercase(),
                        color = when (selectedSystem) {
                            "NES"           -> Color(0xFFE60012)
                            "Super Nintendo" -> Color(0xFF51268F)
                            "Sony PlayStation" -> Color(0xFF00ADB5)
                            else            -> AppleBlue
                        },
                        fontSize = 52.sp, fontWeight = FontWeight.ExtraBold,
                        modifier = Modifier.align(Alignment.Center)
                    )
                    // System Badge
                    Box(
                        Modifier.align(Alignment.BottomEnd).padding(8.dp)
                            .background(Color.Black.copy(alpha = 0.6f), RoundedCornerShape(4.dp))
                            .padding(horizontal = 4.dp, vertical = 2.dp)
                    ) {
                        Text(selectedSystem.take(3).uppercase(),
                            color = Color.Gray, fontSize = 8.sp, fontWeight = FontWeight.Bold)
                    }
                }
            }
            Spacer(Modifier.height(8.dp))
            Text(
                name.substringBeforeLast("."),  // strip extension for display
                color = Color.White, fontSize = 14.sp,
                maxLines = 1, overflow = TextOverflow.Ellipsis,
                textAlign = TextAlign.Center
            )
        }
    }

    private fun handleRomSelection(uri: Uri) {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                // ── 1. Identify the system from the original filename ──────────────
                val originalName =
                    androidx.documentfile.provider.DocumentFile
                        .fromSingleUri(this@MainActivity, uri)?.name
                    ?: uri.lastPathSegment?.substringAfterLast('/') ?: "rom.bin"

                val systemInfo = RomSystemIdentifier.identify(originalName)
                val coreToUse  = systemInfo?.coreName  ?: selectedCore
                val sysToUse   = systemInfo?.systemName ?: selectedSystem

                // ── 2. Copy ROM to internal cache, preserving the extension ───────
                //    The NDK / Libretro cores cannot read content:// URIs directly.
                val cachedRom = File(cacheDir, originalName)
                contentResolver.openInputStream(uri)?.use { input ->
                    cachedRom.outputStream().use { output -> input.copyTo(output) }
                }
                Log.d("OpenEmuCore", "ROM cached: ${cachedRom.absolutePath} (${cachedRom.length()} bytes)")

                // ── 3. Resolve the absolute .so path ─────────────────────────────
                //    Pre-built Libretro cores are in nativeLibraryDir; stub cores
                //    use System.loadLibrary() called later.
                val nativeLibDir = applicationInfo.nativeLibraryDir
                val libretroSoPath: String? = systemInfo?.libretroSo
                    ?.let { File(nativeLibDir, it).takeIf { f -> f.exists() }?.absolutePath }

                withContext(Dispatchers.Main) {
                    selectedSystem = sysToUse
                    selectedCore   = coreToUse
                    isGameRunning  = true

                    // Beta 11: Set pending state instead of booting immediately
                    pendingRomPath = cachedRom.absolutePath
                    pendingCoreName = coreToUse
                    pendingLibretroSo = libretroSoPath
                }
            } catch (e: Exception) {
                Log.e("OpenEmuCore", "ROM selection error", e)
            }
        }
    }

    private fun handleSignInResult(task: com.google.android.gms.tasks.Task<com.google.android.gms.auth.api.signin.GoogleSignInAccount>) {
        try {
            val account = task.getResult(com.google.android.gms.common.api.ApiException::class.java)
            Log.d("OpenEmuCore", "Signed in successfully: ${account.email}")
            // Initialize sync logic
            val sync = GoogleDriveSync(this, account)
            lifecycleScope.launch(Dispatchers.IO) {
                val files = sync.listCloudFiles()
                Log.d("OpenEmuCore", "Cloud Files Found: ${files.size}")
            }
        } catch (e: Exception) {
            Log.e("OpenEmuCore", "Google Sign-In failed", e)
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
                Box(modifier = Modifier.weight(0.6f).fillMaxWidth().background(macOS_Library_Dark)) {
                    OnScreenController(transparent = false)
                }
            }
        }
    }

    @Composable
    fun PauseOverlay(onResume: () -> Unit, onQuit: () -> Unit) {
        Box(
            modifier = Modifier.fillMaxSize(),
            contentAlignment = Alignment.Center
        ) {
            // ── Background Scrim & Blur (Z-Layer 0) ─────────────────────────
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .liquidGlass(50f, 50f)
                    .background(Color.Black.copy(alpha = 0.6f))
                    .clickable(
                        interactionSource = remember { androidx.compose.foundation.interaction.MutableInteractionSource() },
                        indication = null,
                        onClick = onResume // Beta 10 Fix: Tapping background RESUMES game
                    )
            )

            // ── Foreground Menu (Z-Layer 1) ─────────────────────────────────
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(24.dp),
                modifier = Modifier.pointerInput(Unit) { detectTapGestures { /* Block underlying clicks */ } }
            ) {
                Text(
                    "PAUSED",
                    color = Color.White,
                    fontSize = 32.sp,
                    fontWeight = FontWeight.ExtraBold,
                    letterSpacing = 4.sp
                )

                Column(Modifier.width(280.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Button(
                        onClick = onResume,
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(containerColor = AppleBlue)
                    ) {
                        Text("Resume")
                    }
                    Button(
                        onClick = onQuit,
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(containerColor = Color.Transparent),
                        border = ButtonDefaults.outlinedButtonBorder
                    ) {
                        Text("Quit to Library", color = Color.White)
                    }
                }

                // Opacity Slider inside Pause Menu
                Column(Modifier.width(280.dp).padding(top = 32.dp)) {
                    Text("CONTROL OPACITY", color = Color.Gray, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                    Slider(
                        value = controllerOpacity,
                        onValueChange = { controllerOpacity = it },
                        valueRange = 0.0f..1.0f,
                        colors = SliderDefaults.colors(thumbColor = Color.White, activeTrackColor = AppleBlue)
                    )
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
                            
                            // Beta 11: Trigger JNI boot now that the surface is definitely ready
                            val rom = pendingRomPath
                            val core = pendingCoreName
                            val so = pendingLibretroSo
                            
                            if (rom != null && core != null) {
                                lifecycleScope.launch(Dispatchers.IO) {
                                    try {
                                        if (so != null) {
                                            nativeLoadROM(rom, so)
                                        } else {
                                            System.loadLibrary(core)
                                            nativeLoadROM(rom, "")
                                        }
                                        // Clear pending state after successful trigger
                                        pendingRomPath = null
                                        pendingCoreName = null
                                        pendingLibretroSo = null
                                    } catch (e: Exception) {
                                        Log.e("OpenEmuCore", "JNI Boot failed in surfaceCreated", e)
                                    }
                                }
                            }
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
        val color = if (transparent) FrostedGlass else AppleBlue
        
        Box(modifier = Modifier.fillMaxSize()
            .pointerInput(Unit) {
                detectTapGestures(onLongPress = { isPaused = true }) // Long press to pause
            }
        ) {
            if (transparent) {
                // Liquid Glass HUD background
                Box(Modifier.fillMaxSize().liquidGlass(20f, 20f).background(
                    Brush.verticalGradient(listOf(Color.Black.copy(alpha = 0.1f), Color.Black.copy(alpha = 0.3f)))
                ))
            }
            
            // System-Specific Dynamic Layout
            val assetName = when (selectedSystem) {
                "NES" -> "controller_nes.png"
                "Super Nintendo" -> "controller_snes_usa.png"
                "Game Boy Advance" -> "controller_gba.png"
                "Sony PlayStation" -> "controller_psx.png"
                "Nintendo 64" -> "controller_n64.png"
                "Genesis" -> "controller_genesis.png"
                else -> "controller_gba.png"
            }
            
            DynamicControllerLayout(assetName, color)
        }
    }

    @Composable
    fun DynamicControllerLayout(assetName: String, tint: Color) {
        // Here we would implement complex layout logic. For now, we use a generic 
        // layout that uses the system color and potentially shows the asset.
        when (selectedSystem) {
            "NES" -> NESLayout(tint)
            "Super Nintendo" -> SNESLayout(tint)
            "Sony PlayStation" -> PlayStationLayout(tint)
            else -> GBALayout(tint)
        }
        
        // Overlay a Pause Button if not in Landscape
        Box(Modifier.fillMaxSize().padding(16.dp)) {
            IconButton(onClick = { isPaused = true }, modifier = Modifier.align(Alignment.TopCenter).background(Color.White.copy(alpha = 0.05f), CircleShape)) {
                Icon(Icons.Default.Menu, null, tint = Color.White.copy(alpha = 0.3f))
            }
        }
    }

    @Composable
    fun PlayStationLayout(color: Color) {
        // High-fidelity PlayStation controls
        Box(Modifier.fillMaxSize()) {
            // D-Pad (Cross shaped)
            Box(Modifier.align(Alignment.CenterStart).padding(start = 32.dp).size(160.dp)) {
                ControllerButton("U", Color.DarkGray) { sendInput("UP", it) }
                Box(Modifier.align(Alignment.CenterStart)) { ControllerButton("L", Color.DarkGray) { sendInput("LEFT", it) } }
                Box(Modifier.align(Alignment.CenterEnd)) { ControllerButton("R", Color.DarkGray) { sendInput("RIGHT", it) } }
                Box(Modifier.align(Alignment.BottomCenter)) { ControllerButton("D", Color.DarkGray) { sendInput("DOWN", it) } }
            }

            // DualShock Buttons (Diamond with Symbols)
            Box(Modifier.align(Alignment.CenterEnd).padding(end = 32.dp).size(180.dp)) {
                // Triangle
                Box(Modifier.align(Alignment.TopCenter)) { 
                    ControllerButton("△", Color.DarkGray, circle = true) { sendInput("TRIANGLE", it) } 
                }
                // Square
                Box(Modifier.align(Alignment.CenterStart)) { 
                    ControllerButton("□", Color.DarkGray, circle = true) { sendInput("SQUARE", it) } 
                }
                // Circle
                Box(Modifier.align(Alignment.CenterEnd)) { 
                    ControllerButton("○", Color(0xFFE41B17), circle = true) { sendInput("CIRCLE", it) } 
                }
                // Cross
                Box(Modifier.align(Alignment.BottomCenter)) { 
                    ControllerButton("✕", AppleBlue, circle = true) { sendInput("CROSS", it) } 
                }
            }
            
            // L/R Triggers (Shoulders)
            Row(Modifier.align(Alignment.TopCenter).padding(top = 48.dp)) {
                ControllerButton("L1", color) { /* L1 */ }
                Spacer(Modifier.width(120.dp))
                ControllerButton("R1", color) { /* R1 */ }
            }
        }
    }

    @Composable
    fun NESLayout(color: Color) {
        // NES specific hardware layout
        Box(Modifier.fillMaxSize()) {
            // D-Pad
            Column(Modifier.align(Alignment.CenterStart).padding(start = 32.dp)) {
                ControllerButton("U", Color.DarkGray) { sendInput("UP", it) }
                Row {
                    ControllerButton("L", Color.DarkGray) { sendInput("LEFT", it) }
                    Spacer(Modifier.width(10.dp))
                    ControllerButton("R", Color.DarkGray) { sendInput("RIGHT", it) }
                }
                ControllerButton("D", Color.DarkGray) { sendInput("DOWN", it) }
            }
            // Red buttons (NES uses horizontal A/B layout)
            Row(Modifier.align(Alignment.CenterEnd).padding(end = 32.dp)) {
                ControllerButton("B", Color(0xFFE60012), circle = true) { sendInput("B", it) }
                Spacer(Modifier.width(20.dp))
                ControllerButton("A", Color(0xFFE60012), circle = true) { sendInput("A", it) }
            }
        }
    }

    @Composable
    fun SNESLayout(color: Color) {
        // SNES layout (Diamond ABXY)
        Box(Modifier.fillMaxSize()) {
            Column(Modifier.align(Alignment.CenterStart).padding(start = 32.dp)) {
                ControllerButton("U", Color.Gray) { sendInput("UP", it) }
                Row {
                    ControllerButton("L", Color.Gray) { sendInput("LEFT", it) }
                    Spacer(Modifier.width(8.dp))
                    ControllerButton("R", Color.Gray) { sendInput("RIGHT", it) }
                }
                ControllerButton("D", Color.Gray) { sendInput("DOWN", it) }
            }
            
            // SNES Diamond button layout (X top, Y left, B bottom, A right)
            Box(Modifier.align(Alignment.CenterEnd).padding(end = 40.dp).size(160.dp)) {
                Box(Modifier.align(Alignment.TopCenter)) { ControllerButton("X", Color(0xFF51268F), circle = true) { sendInput("X", it) } }
                Box(Modifier.align(Alignment.CenterStart)) { ControllerButton("Y", Color(0xFF8E6EB3), circle = true) { sendInput("Y", it) } }
                Box(Modifier.align(Alignment.CenterEnd)) { ControllerButton("A", Color(0xFF8E6EB3), circle = true) { sendInput("A", it) } }
                Box(Modifier.align(Alignment.BottomCenter)) { ControllerButton("B", Color(0xFF51268F), circle = true) { sendInput("B", it) } }
            }
        }
    }

    @Composable
    fun GBALayout(color: Color) {
        Box(Modifier.fillMaxSize()) {
            // D-Pad (Left side)
            Column(modifier = Modifier.align(Alignment.CenterStart).padding(start = 24.dp)) {
                ControllerButton("U", color) { state -> sendInput("UP", state) }
                Row {
                    ControllerButton("L", color) { state -> sendInput("LEFT", state) }
                    Spacer(modifier = Modifier.width(8.dp))
                    ControllerButton("R", color) { state -> sendInput("RIGHT", state) }
                }
                ControllerButton("D", color) { state -> sendInput("DOWN", state) }
            }

            // Action Buttons (Right side)
            Row(modifier = Modifier.align(Alignment.CenterEnd).padding(end = 24.dp)) {
                ControllerButton("B", color, circle = true) { state -> sendInput("B", state) }
                Spacer(modifier = Modifier.width(16.dp))
                ControllerButton("A", color, circle = true) { state -> sendInput("A", state) }
            }
        }
    }

    @Composable
    fun ControllerButton(text: String, color: Color, circle: Boolean = false, onStateChange: (Boolean) -> Unit) {
        val alphaColor = color.copy(alpha = controllerOpacity)
        Box(
            modifier = Modifier
                .padding(4.dp)
                .size(if (circle) 64.dp else 56.dp)
                .background(alphaColor, if (circle) CircleShape else RoundedCornerShape(8.dp))
                .pointerInput(Unit) {
                    awaitPointerEventScope {
                        while (true) {
                            val event = awaitPointerEvent()
                            when (event.type) {
                                PointerEventType.Press -> onStateChange(true)
                                PointerEventType.Release -> onStateChange(false)
                            }
                        }
                    }
                },
            contentAlignment = Alignment.Center
        ) {
            Text(text, color = Color.White.copy(alpha = controllerOpacity + 0.2f), fontSize = 20.sp)
        }
    }

    @Composable
    fun SettingsScreen(onClose: () -> Unit, onPickFolder: () -> Unit) {
        var selectedTab by remember { mutableStateOf("Library") }
        val biosPickerLauncher = rememberLauncherForActivityResult(
            contract = ActivityResultContracts.OpenDocument(),
            onResult = { uri -> psxBiosPath = uri?.toString() }
        )

        Column(Modifier.fillMaxSize().background(Color.Black)) {
            // Header with Frosted Glass look
            Box(Modifier.fillMaxWidth().height(64.dp).background(FrostedGlass).padding(horizontal = 8.dp), contentAlignment = Alignment.Center) {
                Row(Modifier.fillMaxSize(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                    IconButton(onClick = onClose) { Icon(Icons.Default.ArrowBack, null, tint = Color.White) }
                    Text("Settings", color = Color.White, fontWeight = FontWeight.ExtraBold, fontSize = 18.sp)
                    Box(Modifier.size(48.dp))
                }
            }

            // Tabs with cleaner indicators
            Row(Modifier.fillMaxWidth().background(Color(0xFF151515)), horizontalArrangement = Arrangement.SpaceEvenly) {
                listOf("Library", "Cores", "Controls").forEach { tab ->
                    TextButton(onClick = { selectedTab = tab }, modifier = Modifier.weight(1f)) {
                        Text(tab, color = if (selectedTab == tab) AppleBlue else Color.Gray, 
                             fontWeight = if (selectedTab == tab) FontWeight.Bold else FontWeight.Normal,
                             fontSize = 13.sp)
                    }
                }
            }

            Divider(color = Color.DarkGray, thickness = 0.5.dp)

            Column(Modifier.padding(24.dp).fillMaxSize()) {
                when (selectedTab) {
                    "Library" -> {
                        SettingsHeader("Storage")
                        Text("ROM Root Folder", color = Color.Gray, fontSize = 12.sp)
                        Text(rootFolderUri?.path ?: "No folder selected", color = Color.White, fontSize = 14.sp)
                        Spacer(Modifier.height(12.dp))
                        Button(onClick = onPickFolder, colors = ButtonDefaults.buttonColors(containerColor = AppleBlue), shape = RoundedCornerShape(8.dp)) {
                            Text("Select Folder", fontSize = 13.sp)
                        }
                    }
                    "Cores" -> {
                        SettingsHeader("System Cores")
                        val allSystems = RomSystemIdentifier.getAllSystems()
                        val activeCores = allSystems.filter { it.libretroSo != null }
                        
                        LazyColumn(modifier = Modifier.fillMaxSize()) {
                            items(activeCores) { info ->
                                Column(modifier = Modifier.padding(vertical = 8.dp)) {
                                    Text(info.systemName, color = Color.Gray, fontSize = 12.sp)
                                    Text(info.libretroSo ?: "Internal", color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.Bold)
                                    Divider(color = Color.DarkGray, thickness = 0.5.dp, modifier = Modifier.padding(top = 8.dp))
                                }
                            }
                            
                            item {
                                if (selectedSystem == "Sony PlayStation") {
                                    Spacer(Modifier.height(24.dp))
                                    SettingsHeader("BIOS Management")
                                    Text("PlayStation BIOS (scph5501.bin)", color = Color.Gray, fontSize = 12.sp)
                                    Text(psxBiosPath ?: "Missing", color = if (psxBiosPath != null) Color.Green else Color(0xFFE60012), fontSize = 14.sp)
                                    Button(onClick = { biosPickerLauncher.launch(arrayOf("*/*")) }, modifier = Modifier.padding(top = 8.dp)) {
                                        Text("Upload BIOS")
                                    }
                                }
                            }
                        }
                    }
                    "Controls" -> {
                        SettingsHeader("Touch HUD")
                        Text("Button Opacity", color = Color.Gray, fontSize = 12.sp)
                        Slider(value = controllerOpacity, onValueChange = { controllerOpacity = it }, valueRange = 0.1f..1.0f, colors = SliderDefaults.colors(thumbColor = AppleBlue, activeTrackColor = AppleBlue))
                        Text("${(controllerOpacity * 100).toInt()}%", color = Color.White, textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
                    }
                }

                // Beta 14: Crash Log Path (at the very bottom)
                Spacer(Modifier.weight(1f))
                Box(Modifier.fillMaxWidth().padding(top = 16.dp).background(Color.DarkGray.copy(alpha = 0.2f), RoundedCornerShape(4.dp)).padding(8.dp)) {
                    Column {
                        Text("DEBUG CRASH LOG", color = AppleBlue, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                        Text(getExternalFilesDir(null)?.absolutePath + "/openemu_crash_log.txt", 
                             color = Color.LightGray, fontSize = 9.sp, lineHeight = 12.sp)
                    }
                }
            }
        }
    }

    @Composable
    fun SettingsHeader(title: String) {
        Text(title, color = AppleBlue, fontSize = 11.sp, fontWeight = FontWeight.ExtraBold, letterSpacing = 1.sp, modifier = Modifier.padding(bottom = 8.dp))
    }

    private fun scanFolder(uri: Uri) {
        val root = DocumentFile.fromTreeUri(this, uri) ?: return
        val supportedExtensions = RomSystemIdentifier.getAllSupportedExtensions()
        val foundDocs = mutableListOf<DocumentFile>()
        root.listFiles().forEach { file ->
            val ext = file.name?.substringAfterLast(".", "")?.lowercase()
            if (file.isFile && supportedExtensions.contains(ext)) {
                foundDocs.add(file)
            }
        }
        Log.d("OpenEmuCore", "Scanned ${foundDocs.size} ROMs from $uri")
        scannedGames = foundDocs
    }

    private fun sendInput(button: String, isPressed: Boolean) {
        Log.d("OpenEmuCore", "Input: $button ${if (isPressed) "DOWN" else "UP"}")
        nativeSendInput(button, isPressed)
    }

    private external fun nativeInitLogger(logDir: String)
    private external fun nativeSendInput(button: String, isPressed: Boolean)
    private external fun nativeLoadROM(path: String, system: String)
    private external fun nativeSetSurface(surface: Any?)
    private external fun nativeSetSize(width: Int, height: Int)
    private external fun stringFromJNI(): String

}

@Composable
fun OpenEmuTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = AppleBlue,
            secondary = Color(0xFF51268F),
            tertiary = Color(0xFFE60012),
            background = Color(0xFF000000), // Pure OLED black
            surface = Color(0xFF1C1C1E) // Apple-style dark surface
        ),
        typography = Typography(
            bodyLarge = androidx.compose.ui.text.TextStyle(
                fontFamily = androidx.compose.ui.text.font.FontFamily.SansSerif,
                fontWeight = FontWeight.Medium,
                fontSize = 16.sp
            )
        ),
        content = content
    )
}
