# 1. Introduction

Fake SID is a chiptune tracker that let's you create Commodore 64 music.

A *song* in Fake SID is basically a table with one column per voice.
Table entries are references to tracks.
*Tracks* represent one bar of music for one voice.
They contain notes and references to instruments and effects.
*Instruments* control the volume and waveform of a voice.
They may also control the filter.
*Effects* modify the pitch of a voice.

At the top of the screen you find certain tabs which let you switch to different views.
Let's go through each view and discuss them in more detail.


# 2. Project

Here you set the title, author, track length, and tempo of the current song.
Additionally, songs can be loaded, saved, deleted, and exported.

*Track length* is the number of rows per track.
All tracks of a song have the same length.
As is common with most C64 trackers, time is split into slices of 1/50 of a second, called frames.
*Tempo* is the number of frames spent per track row.
*Swing* is the number additional frames for even-numbered track rows.

Pres *New* to reset the current song.
To load a previously saved song, simply select a song from the song list.
This will enter the song name in the song name input field.
Now press *Load*.
Press *Save* to save the current song under the name in the input field.
Press *Delete* to delete the selected song.
You may render the current song to *WAV* or *OGG* by first selecting the desired file format and then pressing *Export*.
Song files and exported songs are stored in the directories `fakesid/songs/` and `fakesid/exports/`
of your phone's internal shared storage.


# 3. Song

The song table contains references to tracks.
Each column represents a voice.
The original SID - the sound chip of the C64 - has three voices, i.e. you can play three sounds simultaneously.
In Fake SID you have a fourth voice at your disposal, although true purists abstain from using it.

You can mute and unmute voices by pressing the relevant buttons in the table header.

Add and remove rows to the song table by respectively pressing `+` and `-`.
The leftmost column of the table holds indices of the respective table row.

You can set the position at which new rules are inserted and current rows are deleted
by selecting the relevant row index.
This will also set the player position, which is indicated with by highlighted row.

Assign a reference to a table cell by touching it.
This will open up the track select screen with all available track references from `00` to `BK`.
Non-empty tracks are highlighted.
Choose the reference you wish to assign, or touch Clear to clear the table cell.

The three buttons at the bottom are visible in all views.
The first button toggles looping.
Looping will cause the player to repeat the current song row indefinitely.
The second button stops playback and resets the player position.
The third button toggles playback.


# 4. Track

Tracks are tables with three columns for instrument references, effect references, and notes, in that order.
At the top left you find the reference of the current track.
Touch it to open up the track select screen.
(Alternatively, touch the Track tab.)
The arrow buttons let you switch though tracks in sequence.
On the top right are buttons for copying and pasting tracks.

The two rows of buttons on the bottom list references to the most recently used instruments and effects, respectively.
To select an instrument or effect, press the corresponding button.
There are 48 slots for instruments and as many for effects.
To select an instrument from among all available instruments,
hold the Instrument tab to open up the instrument select screen.
The same applies to effects.

The main area of the screen shows the track table and the note matrix.
Use the scrollbars to navigate.
Insert notes by touching the respective cell in the note matrix.
This will also assign the selected instrument and effect.
Changing notes works the same, except that the instrument and effect aren't assigned.
To clear a row, press the note button.
Press it again to insert a note-off event.
Assign and remove an instrument or effect from a track row by pressing the relevant button.
To pick up an instrument or effect, press and hold the button instrument/effect button.


# 5. Instrument

Each instrument has an envelope for volume control, a wavetable, and an optional filter table.

TODO


## 5.1 Wave

TODO


## 5.2 Filter

TODO


# 6. Effect

TODO

+ arpeggio

+ vibrato

+ percussion in combination with instrument
