package org.openemu.android

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val textView = TextView(this)
        textView.id = 1 // Simplified for now
        textView.text = stringFromJNI()
        setContentView(textView)
    }

    external fun stringFromJNI(): String

    companion object {
        init {
            System.loadLibrary("mgba")
        }
    }
}
