[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/-ZR3sVSa)
# a10-cloud

    * Team Number: 13
    * Team Name: BlissAlarm
    * Team Members: Abir Hossain, Mostafa Afr
    * Description of test hardware: (development boards, sensors, actuators, laptop + OS, etc) 

## Github Repo Submission Resources

* [ESE5160 Example Repo Submission](https://github.com/ese5160/example-repository-submission)
* [Markdown Guide: Basic Syntax](https://www.markdownguide.org/basic-syntax/)
* [Adobe free video to gif converter](https://www.adobe.com/express/feature/video/convert/video-to-gif)
* [Curated list of example READMEs](https://github.com/matiassingers/awesome-readme)
* [VS Code](https://code.visualstudio.com/) is heavily recommended to develop code and handle Git commits
  * Code formatting and extension recommendation files come with this repository.
  * Ctrl+Shift+V will render the README.md (maybe not the images though)

## Fill out your assignment answers below

Include all of your relevant source files in the Github repository. You can use Markdown to link GIF images if a video submission is required - or link to a publicly available website hosting the video.

## 1. OTAFU
* We downloaded the TestA.bin file from the server 
* Youtube Link: https://youtu.be/vXE8FULY28Q 

## 2. GPIO Function Timing
* In this part, I ran the test many times, by pressing more than 5 buttons and running the command five separate times. I took more tests than necessary in the video, but these were the times giving me 2.40ms average time to run the function. It’s worth noting I ran the test another time using a different pin and got 5.5ms average, and I fear something may be off in my hardware, but overall the time seems to be between 2 and 8ms. I just used 7 points from the many samples I took to calculate the average. 
* Youtube link: https://youtu.be/xw529gj1pI4
* Document: https://docs.google.com/document/d/12htPBjGJj3dw36v9CYLqewzUUKuOx4SQcZVw35YgUd8/edit?usp=sharing
* Capture file is on the top-level directory

## 3. Node-RED Design

* Node-RED Design Document: https://docs.google.com/document/d/1zOd9LYXB0QNu_r0FcpbXMWj4PoJWyzbE5rAx0C0kuIs/edit?usp=sharing 

## 4. Node-RED Implementation

* Video demonstration: https://drive.google.com/file/d/131JNsRAgdrDlOWiWm68oTxO7qCRHxsoC/view?usp=sharing
* Screenshots of frontend and backend: https://drive.google.com/drive/folders/1nkbKQjZ9SZYl2AhMxn_ENqXBz7BVWMkv?usp=sharing
* Code in JSON format: Files can be found in Node-RED folder from top directory
* Node-RED: http://20.102.106.224:1880/
* Node-RED Dasboard UI: http://20.102.106.224:1880/ui
