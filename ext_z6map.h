! ext_z6map.h - a map view for z6 games with pictures
!
! Infocom's Arthur shows a picture of the current location in a window at the
! top of the screen, and F2 swaps it for a map of the rooms around the player
! (F1 swaps back). This extension does the same for a PunyInform game: it
! draws the map out of small tile pictures (a room, the current room, and a
! connector for each direction) into the game's picture window.
!
! The map is laid out as the player sees it. The current room is in the middle,
! and the rooms the player has visited are placed around it by following the exits,
! one grid step per exit. An exit leading somewhere the player has not been
! yet is drawn as a connector with no room at the end.
!
! Exits are read, never followed. If desired, an exit that is a
! routine can be given its destination with the map_links property:
!
!     Room Yard "Castle Yard"
!         with e_to [; ... return Harbour; ],
!              map_links e_to Harbour;
!
! map_links holds pairs of direction property and room. It overrides the
! direction property, so "map_links n_to 0" hides an exit from the map.
! Up, down, in and out are not drawn.
!
! Usage:
!
!   1. The game needs a picture window (any z6 window) and these pictures in
!      its blorb. All the tiles must be the same size; the connector tiles
!      are drawn in the cells between the rooms, so they should reach the
!      edges of the tile:
!         Z6MAP_PIC_BACKDROP  drawn first, at the window's top left (optional)
!         Z6MAP_PIC_CURRENT   the room the player is in
!         Z6MAP_PIC_ROOM      any other visited room
!         Z6MAP_PIC_DOOR      a closed door, in place of a connector
!         Z6MAP_PIC_EW        east-west connector   -
!         Z6MAP_PIC_NS        north-south connector |
!         Z6MAP_PIC_NW        nw-se connector       \
!         Z6MAP_PIC_SW        sw-ne connector       /
!      Define these constants, and Z6MAP_WINDOW, before including this file.
!
!   2. Define a routine Z6MapDrawPicture that draws the picture view (whatever
!      the game drew in the picture window before it had a map), and call
!      Z6MapUpdate() wherever the picture used to be drawn - usually from
!      LookRoutine. Z6MapUpdate draws the map or calls Z6MapDrawPicture,
!      depending on the view the player has chosen.
!
!   3. Include this file after ext_z6graphics.h and BEFORE puny.h, since the
!      library looks for HandleFunctionKey while it is being compiled. (If
!      the game has its own HandleFunctionKey, define Z6MAP_NO_KEYS and call
!      Z6MapKey(key) from it instead.)
!
!   4. Optionally add verbs after puny.h, for players and interpreters with no
!      function keys (see vanyar.inf for examples):
!         Verb meta 'map'                    * -> MapView;
!         Verb meta 'picture' 'pictures'     * -> PictureView;

System_file;

#Iffalse (#version_number == 6);
Message fatalerror "ext_z6map can only be used in -v6 mode";
#Endif;

#Ifndef Z6MAP_WINDOW;
Message fatalerror "ext_z6map needs Z6MAP_WINDOW and the Z6MAP_PIC_ constants";
#Endif;

#Ifndef Z6MAP_PIC_BACKDROP;
Constant Z6MAP_PIC_BACKDROP = 0;
#Endif;

#Ifndef Z6MAP_KEY_PICTURE;
Constant Z6MAP_KEY_PICTURE = 133;   ! F1
#Endif;
#Ifndef Z6MAP_KEY_MAP;
Constant Z6MAP_KEY_MAP = 134;       ! F2
#Endif;
#Ifndef Z6MAP_KEY_MAP_PETSCII;
Constant Z6MAP_KEY_MAP_PETSCII = 137; ! the F2 key on a Commodore machine
#Endif;

#Ifndef Z6MAP_NO_KEYS;
Zcharacter terminating 133 134 137;
#Endif;

! The margins of the backdrop that the map must stay clear of, in 320ths of
! the window width and 200ths of its height (the default is Vanyar's scroll,
! whose rolled ends are 12 pixels of a 320 pixel picture).
#Ifndef Z6MAP_MARGIN_X;
Constant Z6MAP_MARGIN_X = 12;
#Endif;
#Ifndef Z6MAP_MARGIN_Y;
Constant Z6MAP_MARGIN_Y = 4;
#Endif;

! The largest map, in tiles. A room takes a tile and so does the connector
! after it, so 31 tiles across is 15 rooms (or 16, see Z6MapLayout).
Constant Z6MAP_MAX_TX = 31;
Constant Z6MAP_MAX_TY = 15;
Constant Z6MAP_MAX_ROOMS = ((Z6MAP_MAX_TX + 1) / 2) * ((Z6MAP_MAX_TY + 1) / 2);

Property map_links;

Global z6map_active;    ! true while the player has chosen the map view

! What is drawn in each tile of the map, filled in before anything is drawn
Constant Z6MAP_T_EMPTY   = 0;
Constant Z6MAP_T_CURRENT = 1;
Constant Z6MAP_T_ROOM    = 2;
Constant Z6MAP_T_DOOR    = 3;
Constant Z6MAP_T_EW      = 4;
Constant Z6MAP_T_NS      = 5;
Constant Z6MAP_T_NW      = 6;
Constant Z6MAP_T_SW      = 7;

Array z6map_tile -> Z6MAP_MAX_TX * Z6MAP_MAX_TY;
Array z6map_room --> Z6MAP_MAX_ROOMS;  ! the room in each room slot, or 0
Array z6map_queue --> Z6MAP_MAX_ROOMS; ! room slots still to follow exits from

! The eight directions, properties, grid step, and the connector between the
! two rooms. The connector tile is half a step away, so it is the same for a
! direction and its opposite.
Array z6map_dir_prop --> n_to s_to e_to w_to ne_to nw_to se_to sw_to;
Array z6map_dir_dx --> 0 0 1 (-1) 1 (-1) 1 (-1);
Array z6map_dir_dy --> (-1) 1 0 0 (-1) (-1) 1 1;
Array z6map_dir_tile -> Z6MAP_T_NS Z6MAP_T_NS Z6MAP_T_EW Z6MAP_T_EW
                        Z6MAP_T_SW Z6MAP_T_NW Z6MAP_T_NW Z6MAP_T_SW;

! The layout, worked out from the window and tile sizes by Z6MapLayout
Global z6map_tx;        ! map width in tiles
Global z6map_ty;        ! map height in tiles
Global z6map_rx;        ! room slots across
Global z6map_ry;        ! room slots down
Global z6map_ox;        ! the tile column of the first room slot (0 or 1)
Global z6map_oy;        ! the tile row of the first room slot (0 or 1)
Global z6map_x0;        ! where tile (0,0) goes in the window, in units
Global z6map_y0;
Global z6map_tw;        ! tile size in units
Global z6map_th;
Global z6map_door;      ! set by Z6MapExit when the exit goes through a door

! Work out how big the map can be. Rooms are on every other tile, with a
! connector tile between two rooms, and the current room must be in the
! middle - so the number of room slots across is odd, and the map is 4n+1
! tiles wide. If there is room for one more tile on each side, the map gets
! it for the connectors that lead out of the view.
[ Z6MapTiles p_avail _r;
    _r = (p_avail - 1) / 4;
    if(_r < 0) _r = 0;
    return _r;
];

[ Z6MapLayout _w _h _mx _my _n _r;
    if(Z6PictureExists(Z6MAP_PIC_ROOM) == false) rfalse;
    z6map_th = z6_picture_info-->0;
    z6map_tw = z6_picture_info-->1;
    if(z6map_tw < 1 || z6map_th < 1) rfalse;
    _w = Z6GetWindowProp(Z6MAP_WINDOW, Z6_WP_X_SIZE);
    _h = Z6GetWindowProp(Z6MAP_WINDOW, Z6_WP_Y_SIZE);
    ! The backdrop may be taller than the window (it is clipped); only what is
    ! visible of it counts.
    if(Z6MAP_PIC_BACKDROP && Z6PictureExists(Z6MAP_PIC_BACKDROP)) {
        if(z6_picture_info-->0 < _h) _h = z6_picture_info-->0;
        if(z6_picture_info-->1 < _w) _w = z6_picture_info-->1;
    }
    _mx = (_w * Z6MAP_MARGIN_X) / 320;
    _my = (_h * Z6MAP_MARGIN_Y) / 200;

    _n = (_w - _mx - _mx) / z6map_tw;
    if(_n > Z6MAP_MAX_TX) _n = Z6MAP_MAX_TX;
    _r = Z6MapTiles(_n);
    z6map_rx = _r + _r + 1;
    z6map_tx = _r * 4 + 1;
    z6map_ox = 0;
    if(_n >= z6map_tx + 2) { z6map_tx = z6map_tx + 2; z6map_ox = 1; }

    _n = (_h - _my - _my) / z6map_th;
    if(_n > Z6MAP_MAX_TY) _n = Z6MAP_MAX_TY;
    _r = Z6MapTiles(_n);
    z6map_ry = _r + _r + 1;
    z6map_ty = _r * 4 + 1;
    z6map_oy = 0;
    if(_n >= z6map_ty + 2) { z6map_ty = z6map_ty + 2; z6map_oy = 1; }

    ! Centre the map in the window (window coordinates are 1-based)
    z6map_x0 = 1 + (_w - z6map_tx * z6map_tw) / 2;
    z6map_y0 = 1 + (_h - z6map_ty * z6map_th) / 2;
    rtrue;
];

! Where an exit from a door leads, without calling anything.
[ Z6MapDoorDest p_door p_room _v _a;
    _v = p_door.door_to;
    if(_v ~= 0 && metaclass(_v) == Object) return _v;
    if(p_door provides found_in && p_door.#found_in == 2 * WORDSIZE) {
        _a = p_door.&found_in;
        if(_a-->0 == p_room && metaclass(_a-->1) == Object) return _a-->1;
        if(_a-->1 == p_room && metaclass(_a-->0) == Object) return _a-->0;
    }
    return -1;
];

! Where the exit from p_room along p_prop leads: 0 if there is no exit, -1 if
! there is one but where it goes can't be known, otherwise the room.
! z6map_door is set to the door if the exit goes through one.
[ Z6MapExit p_room p_prop _v _i _n _a;
    z6map_door = 0;
    if(p_room provides map_links) {
        _n = p_room.#map_links / WORDSIZE;
        _a = p_room.&map_links;
        for(_i = 0 : _i + 1 < _n : _i = _i + 2)
            if(_a-->_i == p_prop) return _a-->(_i + 1);
    }
    @get_prop p_room p_prop -> _v;
    if(_v == 0) return 0;
    switch(metaclass(_v)) {
    String:
        return 0;
    Object:
        if(_v has door) {
            z6map_door = _v;
            return Z6MapDoorDest(_v, p_room);
        }
        return _v;
    }
    return -1;
];

[ Z6MapTileAt p_x p_y;
    return z6map_tile + p_y * z6map_tx + p_x;
];

! Put a connector in a tile, unless it is outside the map. Two diagonals
! can cross in one tile; the first one drawn stays, and a closed door beats a
! connector.
[ Z6MapLink p_x p_y p_tile _a;
    if(p_x < 0 || p_y < 0 || p_x >= z6map_tx || p_y >= z6map_ty) return;
    _a = Z6MapTileAt(p_x, p_y);
    if(_a->0 == Z6MAP_T_EMPTY || p_tile == Z6MAP_T_DOOR) _a->0 = p_tile;
];

! Fill in z6map_tile for a map centred on p_room.
[ Z6MapBuild p_room _i _n;
    _n = z6map_tx * z6map_ty;
    for(_i = 0 : _i < _n : _i++) z6map_tile->_i = Z6MAP_T_EMPTY;
    _n = z6map_rx * z6map_ry;
    for(_i = 0 : _i < _n : _i++) z6map_room-->_i = 0;
    Z6MapPlaceRooms(p_room);
    Z6MapPlaceTiles(p_room);
];

! Place the rooms in z6map_room: breadth first from the current room, through
! visited rooms only, taking the first place each room is reached at.
[ Z6MapPlaceRooms p_room _i _n _head _tail _slot _sx _sy _room _d _dest _nx _ny
        _nslot;
    _n = z6map_rx * z6map_ry;
    _slot = (z6map_ry / 2) * z6map_rx + z6map_rx / 2;
    z6map_room-->_slot = p_room;
    z6map_queue-->0 = _slot;
    _tail = 1;
    for(_head = 0 : _head < _tail : _head++) {
        _slot = z6map_queue-->_head;
        _room = z6map_room-->_slot;
        _sx = _slot % z6map_rx;
        _sy = _slot / z6map_rx;
        for(_d = 0 : _d < 8 : _d++) {
            _dest = Z6MapExit(_room, z6map_dir_prop-->_d);
            if(_dest == 0 or -1 || _dest hasnt visited) continue;
            _nx = _sx + z6map_dir_dx-->_d;
            _ny = _sy + z6map_dir_dy-->_d;
            if(_nx < 0 || _ny < 0 || _nx >= z6map_rx || _ny >= z6map_ry)
                continue;
            _nslot = _ny * z6map_rx + _nx;
            if(z6map_room-->_nslot ~= 0) continue;
            for(_i = 0 : _i < _n : _i++) if(z6map_room-->_i == _dest) break;
            if(_i < _n) continue; ! already on the map somewhere else
            z6map_room-->_nslot = _dest;
            z6map_queue-->_tail = _nslot;
            _tail++;
        }
    }
];

! Put the rooms and their connectors in z6map_tile. A connector is drawn to
! the room the exit leads to if that room is in the next slot, or into an
! empty slot (an exit to somewhere unvisited, or somewhere the map could not
! put there) - never towards a different room, which would be a lie.
[ Z6MapPlaceTiles p_room _n _slot _sx _sy _room _d _dest _nx _ny _tile _other;
    _n = z6map_rx * z6map_ry;
    for(_slot = 0 : _slot < _n : _slot++) {
        _room = z6map_room-->_slot;
        if(_room == 0) continue;
        _sx = _slot % z6map_rx;
        _sy = _slot / z6map_rx;
        _tile = Z6MAP_T_ROOM;
        if(_room == p_room) _tile = Z6MAP_T_CURRENT;
        (Z6MapTileAt(z6map_ox + _sx * 2, z6map_oy + _sy * 2))->0 = _tile;
        for(_d = 0 : _d < 8 : _d++) {
            _dest = Z6MapExit(_room, z6map_dir_prop-->_d);
            if(_dest == 0) continue;
            _nx = _sx + z6map_dir_dx-->_d;
            _ny = _sy + z6map_dir_dy-->_d;
            _other = 0;
            if(_nx >= 0 && _ny >= 0 && _nx < z6map_rx && _ny < z6map_ry)
                _other = z6map_room-->(_ny * z6map_rx + _nx);
            if(_other ~= 0 && _other ~= _dest) continue;
            _tile = z6map_dir_tile->_d;
            if(z6map_door && z6map_door hasnt open) _tile = Z6MAP_T_DOOR;
            Z6MapLink(z6map_ox + _sx * 2 + z6map_dir_dx-->_d,
                z6map_oy + _sy * 2 + z6map_dir_dy-->_d, _tile);
        }
    }
];

[ Z6MapTilePicture p_tile;
    switch(p_tile) {
    Z6MAP_T_CURRENT: return Z6MAP_PIC_CURRENT;
    Z6MAP_T_ROOM:    return Z6MAP_PIC_ROOM;
    Z6MAP_T_DOOR:    return Z6MAP_PIC_DOOR;
    Z6MAP_T_EW:      return Z6MAP_PIC_EW;
    Z6MAP_T_NS:      return Z6MAP_PIC_NS;
    Z6MAP_T_NW:      return Z6MAP_PIC_NW;
    Z6MAP_T_SW:      return Z6MAP_PIC_SW;
    }
    return 0;
];

! Draw the map of the rooms around p_room in the map window. With 0 (in the
! dark, say) only the backdrop is drawn.
[ Z6MapDraw p_room _x _y _pic _px _py _one;
    _one = 1;
    Z6SetWindow(Z6MAP_WINDOW);
    Z6EraseWindow(Z6MAP_WINDOW);
    if(Z6MAP_PIC_BACKDROP) @draw_picture Z6MAP_PIC_BACKDROP _one _one;
    if(p_room && Z6MapLayout()) {
        Z6MapBuild(p_room);
        for(_y = 0 : _y < z6map_ty : _y++) {
            _py = z6map_y0 + _y * z6map_th;
            for(_x = 0 : _x < z6map_tx : _x++) {
                _pic = Z6MapTilePicture((Z6MapTileAt(_x, _y))->0);
                if(_pic == 0) continue;
                _px = z6map_x0 + _x * z6map_tw;
                @draw_picture _pic _py _px;
            }
        }
    }
    Z6SetWindow(0);
];

! Draw whichever view the player has chosen. Call this where the game used to
! draw its picture.
[ Z6MapUpdate;
    if(z6map_active) {
        if(location == thedark) Z6MapDraw(0);
        else Z6MapDraw(real_location);
    } else
        Z6MapDrawPicture();
];

! Switch views. Returns true if the key was one of ours.
[ Z6MapKey p_key;
    if(p_key == Z6MAP_KEY_PICTURE) {
        if(z6map_active) { z6map_active = false; Z6MapUpdate(); }
        rtrue;
    }
    if(p_key == Z6MAP_KEY_MAP or Z6MAP_KEY_MAP_PETSCII) {
        z6map_active = ~~z6map_active;
        Z6MapUpdate();
        rtrue;
    }
    rfalse;
];

#Ifndef Z6MAP_NO_KEYS;
[ HandleFunctionKey p_key;
    return Z6MapKey(p_key);
];
#Endif;

[ MapViewSub;
    if(z6map_active) "You are already looking at the map.";
    z6map_active = true;
    Z6MapUpdate();
    "[Showing the map. F1 or ~picture~ shows the picture again.]";
];

[ PictureViewSub;
    if(z6map_active == false) "You are already looking at the picture.";
    z6map_active = false;
    Z6MapUpdate();
    "[Showing the picture. F2 or ~map~ shows the map again.]";
];
