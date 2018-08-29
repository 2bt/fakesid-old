# 1. Introduction

Fake SID is a chiptune tracker that let's you create Commodore 64 music.
Unlike other C64 trackers, songs made with Fake SID cannot be played on real C64 directly,
but this is mainly an export issue.

A *song* in Fake SID is basically a table with one column per voice.
Table entries are references to tracks.
*Tracks* represent one bar of music for one voice.
They contain notes and references to instruments and effects.
*Instruments* control the volume and waveform of a voice.
They may also control the filter.
*Effects* modify the pitch of a voice.

At the top of the screen you find certain tabs.
Let's go through each and discuss them in more detail.


# 2. Project

Here you set the title, author, track length, and tempo of the current song.
Additionally, songs can be loaded, saved, deleted, and exported.

*Track length* is the number of rows per track.
All tracks of a song have the same length.
As is common with most C64 trackers, time is split into slices of 1/50 of a second, called frames.
*Tempo* is the number of frames spent per track row.
*Swing* is the number additional frames for even-numbered track rows.

To load a previously saved song, simply select a song  from the song list.
This will enter the song name in the song name input field.
Now press *Load*.
Note, that the current song will be lost.
Press *Save* to save the current song, silently overriding the song with the same name, should there be one.
Change the song name before saving if you don't want to override the song.
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
You can select the position at which new rules are inserted and current rows are deleted
by selecting the relevant row index on the table's left.

Assign a reference to a table cell by touching it.
This will open up the track select screen with all available track references from `00` to `BK`.
Non-empty tracks are highlighted.
Choose the reference you wish to assign, or touch *Clear* to clear the table cell.

The three buttons at the bottom are visible in all tabs.
The leftmost button play/pause
+ repeat, stop, and  at the bottom


# 4. Track


# 5. Instrument

Each instrument has an envelope for volume control, a wavetable, and, optionally, a filter table.


# 6. Effect

+ arpeggio

+ vibrato

+ percussion in combination with instrument
