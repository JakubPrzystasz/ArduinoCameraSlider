package com.example.android.myapplication;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import static java.lang.Double.parseDouble;



public class MainActivity extends AppCompatActivity {

    EditText editText;
    Button button;
    RadioButton rb1,rb2;
    RadioGroup group;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        editText = findViewById(R.id.editText);
        button = findViewById(R.id.button);
        rb1 = findViewById(R.id.radioButton);
        rb2 = findViewById(R.id.radioButton2);

        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                String x = editText.getText().toString();
                String z;
                int y,p;
                y = 0;
                int o=x.length()-1;
                for(int i=x.length()-1;i>=0;i--){
                    z = String.valueOf(x.charAt(i));
                    p = Integer.parseInt(z);
                    y = y + (int) p * (int)Math.pow(2,o-i);
                }
                editText.setText(String.valueOf(y));

                /*
                double y = parseDouble(x);
                if(rb1.isChecked()){
                    y = Math.sin(y);
                }
                if(rb2.isChecked()){
                    y = Math.sin(Math.toRadians(y));
                }
                x = String.valueOf(y);
                editText.setText(String.valueOf(x));*/

//                String text = editText.getText().toString();
//                int i = text.length()-1;
//                if(i >= 0) {
//                    String txt = "";
//                    txt = String.valueOf(text.charAt(i));
//                    String x = String.valueOf(text.substring(0, i));
//                    editText.setText(x);
//
//                    editText.setSelection(i);
//                    //for(int i=0;i+1<text.length();i++){
//                }else {
//                    Toast.makeText(getApplicationContext(), "Wpisz coś w pole!",
//                            Toast.LENGTH_SHORT).show();
//                }
//                }

//                for(int i=0;i+1<text.length();i++){
//                    txt = txt + text.charAt(i);
//                }
//                editText.setText(txt);
            }
            });
    }}
