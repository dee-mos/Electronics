void button_1_cb() 
{
  EncButton b = button_1;

    Serial.print("callback: ");
    switch (b.action()) {
        case EB_PRESS:
            Serial.println("press");
            client.publish(hub.topicHub().c_str(),"button_1");
            break;
        case EB_HOLD:
            Serial.println("hold");
            break;
        case EB_STEP:
            Serial.println("step");
            break;
        case EB_RELEASE:
            Serial.print("release. steps: ");
            Serial.print(b.getSteps());
            Serial.print(", press for: ");
            Serial.print(b.pressFor());
            Serial.print(", hold for: ");
            Serial.print(b.holdFor());
            Serial.print(", step for: ");
            Serial.println(b.stepFor());
            break;
        case EB_CLICK:
            Serial.println("click");
            break;
        case EB_CLICKS:
            Serial.print("clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_TURN:
            Serial.print("turn ");
            Serial.print(b.dir());
            Serial.print(" ");
            Serial.print(b.fast());
            Serial.print(" ");
            Serial.println(b.pressing());
            break;
        case EB_REL_HOLD:
            Serial.println("release hold");
            break;
        case EB_REL_HOLD_C:
            Serial.print("release hold clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_REL_STEP:
            Serial.println("release step");
            break;
        case EB_REL_STEP_C:
            Serial.print("release step clicks ");
            Serial.println(b.getClicks());
            break;
        default:
            Serial.println();
    }
}

void button_2_cb() 
{
  EncButton b = button_2;

    Serial.print("callback: ");
    switch (b.action()) {
        case EB_PRESS:
            Serial.println("press");
            client.publish(hub.topicHub().c_str(),"button_2");
            break;
        case EB_HOLD:
            Serial.println("hold");
            break;
        case EB_STEP:
            Serial.println("step");
            break;
        case EB_RELEASE:
            Serial.print("release. steps: ");
            Serial.print(b.getSteps());
            Serial.print(", press for: ");
            Serial.print(b.pressFor());
            Serial.print(", hold for: ");
            Serial.print(b.holdFor());
            Serial.print(", step for: ");
            Serial.println(b.stepFor());
            break;
        case EB_CLICK:
            Serial.println("click");
            break;
        case EB_CLICKS:
            Serial.print("clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_TURN:
            Serial.print("turn ");
            Serial.print(b.dir());
            Serial.print(" ");
            Serial.print(b.fast());
            Serial.print(" ");
            Serial.println(b.pressing());
            break;
        case EB_REL_HOLD:
            Serial.println("release hold");
            break;
        case EB_REL_HOLD_C:
            Serial.print("release hold clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_REL_STEP:
            Serial.println("release step");
            break;
        case EB_REL_STEP_C:
            Serial.print("release step clicks ");
            Serial.println(b.getClicks());
            break;
        default:
            Serial.println();
    }
}
