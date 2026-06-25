.. image:: .github/obs-multistream-logo.png
   :alt: OBS MultiStream
   :width: 160
   :align: center

OBS MultiStream
===============

OBS MultiStream — native multi-destination streaming, a fork of `OBS Studio
<https://obsproject.com>`_.

What is OBS MultiStream?
------------------------

OBS MultiStream is an OBS Studio fork that streams one composited production
to many destinations at once. Instead of a single output, it builds on
multiple canvases (each with its own resolution, FPS, and encoder settings)
and multiplexes the encoded output to every enabled destination bound to a
canvas — encode once per canvas, fan out to many. The desktop UI is a
CEF-hosted Svelte frontend.

It is built on `OBS Studio <https://obsproject.com>`_ as its upstream base,
inheriting its capture, compositing, encoding, and plugin ecosystem.

It's distributed under the GNU General Public License v2 (or any later
version) - see the accompanying COPYING file for more details.

Upstream OBS Studio resources
-----------------------------

The following are genuine OBS Studio (upstream) resources. They cover the
underlying engine and contribution process, not fork-specific workflows:

- OBS Studio website: https://obsproject.com

- OBS Studio Help/Documentation/Guides: https://github.com/obsproject/obs-studio/wiki

- OBS Studio Forums: https://obsproject.com/forum/

- OBS Studio Build Instructions: https://github.com/obsproject/obs-studio/wiki/Install-Instructions

- OBS Studio Developer/API Documentation: https://obsproject.com/docs

- OBS Studio Bug Tracker: https://github.com/obsproject/obs-studio/issues

Contributing
------------

- OBS MultiStream is a fork. Coding and commit guidelines, code style, and
  the code of conduct are inherited from OBS Studio (upstream):

  - Coding and commit guidelines:
    https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.md

  - Code style guidelines:
    https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md

  - Code of Conduct:
    https://github.com/obsproject/obs-studio/blob/master/COC.rst

- Developer/API documentation for the underlying engine can be found here:
  https://obsproject.com/docs

- If you would like to help fund or sponsor upstream OBS Studio, you can do
  so via `Patreon <https://www.patreon.com/obsproject>`_, `OpenCollective
  <https://opencollective.com/obsproject>`_, or `PayPal
  <https://www.paypal.me/obsproject>`_.  See the OBS Studio `contribute page
  <https://obsproject.com/contribute>`_ for more information.

SAST Tools
----------

`PVS-Studio <https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source>`_ - static analyzer for C, C++, C#, and Java code.
