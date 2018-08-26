# Fake SID

This is a chiptune tracker for android that's trying to sound a lot like the MOS Technology SID.
The synth code is based on TinySID by Tammo Hinrichs (kb).
Some details of the SID chip aren't emulated correctly but it's close enough, I think.
Also, I added a fourth voice (but you don't have to use it if you don't want to, of course).
The tracker is highly inspired by [lft's Blackbird](https://csdb.dk/release/?id=161554).

Listen to the demo songs on [SoundCloud](https://soundcloud.com/daniel-langner-150098802/sets/fake-sid-demo-songs).

Get the app from the [Google Play Store](https://play.google.com/store/apps/details?id=com.twobit.fakesid).


### Screeshots

![image](screenshots/0.png)
![image](screenshots/1.png)
![image](screenshots/2.png)
![image](screenshots/3.png)
![image](screenshots/4.png)
![image](screenshots/5.png)


### How to build

In order to compile for android the following setup is prerequisite:

	$ ls ..
	android-ndk-r16     # android NDK
	fakesid             # the project folder
	libsndfile          # libsndfile source (TODO: provide Android.mk)
	SDL2-2.0.7          # SDL source
	SDL2_image-2.0.2    # SDL image source
	SDL2_mixer-2.0.2    # SDL mixer source
	tools_r25.2.5-linux # android SDK

Once you have that, run `make` like so:

	make android-install


### TODO

+ confirm load/save/delete
+ documentation
