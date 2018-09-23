## tagmage

Save, organize, and display your images via tags.

**Note:** This project is still immature, so expect breaking changes
between major versions and in the master branch!

### Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Quickstart](#quickstart)
- [Using Tad](#using-tad)

### Installation

To install, download [the latest
release](https://github.com/samuel-hunter/tagmage/releases). Edit `config.mk` to
your tastes, install the required dependencies and compile:

    $ sudo apt-get install libsqlite3-dev libbsd-dev
    $ make
    $ sudo make install

### Usage

    Usage: tagmage [ -f PATH ] COMMAND [ ... ]
    
      -f SAVE  - Set custom save directory.
    
      add [-t TAG1,TAG2,...] IMAGES..
      edit IMAGE TITLE
      list [TAGS..]
      untagged
      tag IMAGE [TAGS..]
      untag IMAGE [TAGS..]
      tags IMAGE
      path [IMAGES..]
      rm IMAGES..
    
    Visit `man 1 tagmage` for more details.

The frontend utility `tad` also has its own manpage.

### Quickstart

We'll be working with `tagmage` to save and manage images, and `tad` to list
them.

We have several images in `~` named `a.png`, `b.png`, ..., `g.png`. Let's add
`a.png` to the database:

    $ tagmage add a.png
    1

The image is now copied to `$XDG_DATA_HOME/tagmage`, and we received the id of
our new image. To get the path of where the image is located:

    $ tagmage path 1
    /home/$USER/.local/share/tagmage/1.png

Listing all the images, we see:

    $ tagmage list
    1 a.png

A database with one file and no tags isn't much of a database, though. Let's add
the rest:

    $ tagmage add -t foo,bar b.png
    2
    $ tagmage add -t bar,qux {c..g}.png
    3
    4
    5
    6
    7

Not only can we tag an image the same line that we add it to the database, but
we can also add multiple images with the same tags on the same line. Now that we
have tags, we can now list all tags that we have, and also list the tags that a
single image has:

    $ tagmage tags
    foo
    bar
    qux
    $ tagmage tags 3
    bar
    qux

We can also list all images that only have certain tags:

    $ tagmage list foo
    2 b.png
    $ tagmage list bar qux
    3 c.png
    4 d.png
    5 e.png
    6 f.png
    7 g.png

To add or remove tags to existing images:

    $ tagmage tag 2 bux
    $ tagmage tags 2
    foo
    bar
    bux
    $ tagmage untag 2 bux
    foo
    bar

To rename an existing image:

    $ tagmage edit 2 an_image.png
    $ tagmage list foo
    2 an_image.png
    
### Using Tad

These utilities are useful for a frontend, but not that exciting if you want to
view your images in bulk. Let's use `tad` to create a directory full of files
linked to the database:

    $ tad list

By default, `tad` created a directory at `/tmp/tad-$USER`:

    $ ls /tmp/tad-$USER
    00001-a.png
    00002-animage.png
    00003-c.png
    00004-d.png
    00005-e.png
    00006-e.png
    00007-e.png
    $ readlink /tmp/tad-$USER/00001-a.png
    /home/$USER/.local/share/tagmage/1

We can filter by tags the same way:

    $ tad list foo
    $ ls /tmp/tad-$USER
    00002-an_image.png
    $ tad list bar qux
    $ ls /tmp/tad-$USER
    00003-c.png
    00004-d.png
    00005-e.png
    00006-f.png
    00007-g.png

Common utilities found in `tagmage` have their own version in `tad` as well:

    $ tad list && cd /tmp/tad-$USER
    $ tad edit 00002-an_image.png b # The file is now renamed at this point.
    $ readlink 00002-b.png
    /home/$USER/.local/share/tagmage/2
    $ tad tag 00002-b.png qux
    $ tad untag 00002-b.png qux
    $ tad rm 00003-c.png
    $ # The symlink should be automatically removed.
    $ ls 00003-c.png
    ls: cannot access '00003-c.png': No such file or directory.

Note that if you delete the file with `rm`, and not `tad rm` or `tagmage rm`,
the changes will not be reflected in the database, and will appear again next
time you list images.

You are also able to list images a separate way:

    $ tad tags
    $ tree /tmp/tad-$USER
    /tmp/tad-$USER
    ├── foo
    │   └── 00002-b.png -> /home/$USER/.local/share/tagmage/2
    ├── bar
    │   ├── 00002-b.png -> /home/$USER/.local/share/tagmage/2
    │   ├── 00003-c.png -> /home/$USER/.local/share/tagmage/3
    # ...and so on.

If you call `tad tags` without any other arguments, it will also create a folder
called `:untagged` that lists all images without any tags.

This directory tree is easier to look at with the human eye, but others may
prefer `tad list`, or it might be easier to run a script in the simpler
directory tree.
        
