package org.openemu.android

import android.content.Context
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInAccount
import com.google.api.client.googleapis.extensions.android.gms.auth.GoogleAccountCredential
import com.google.api.client.http.javanet.NetHttpTransport
import com.google.api.client.json.gson.GsonFactory
import com.google.api.services.drive.Drive
import com.google.api.services.drive.DriveScopes
import java.util.Collections

class GoogleDriveSync(private val context: Context, private val account: GoogleSignInAccount) {
    
    private val driveService: Drive by lazy {
        val credential = GoogleAccountCredential.usingOAuth2(
            context, Collections.singleton(DriveScopes.DRIVE_APPDATA)
        )
        credential.selectedAccount = account.account
        
        Drive.Builder(
            NetHttpTransport(),
            GsonFactory(),
            credential
        ).setApplicationName("OpenEmuARM64").build()
    }
    
    fun listCloudFiles(): List<String> {
        val googleDriveFiles = mutableListOf<String>()
        try {
            val result = driveService.files().list()
                .setSpaces("appDataFolder")
                .setFields("nextPageToken, files(id, name, modifiedTime)")
                .execute()
            
            for (file in result.files) {
                googleDriveFiles.add("${file.name} (${file.modifiedTime})")
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
        return googleDriveFiles
    }
}
