create table if not exists utc_system (
	id         integer primary key,
	db_version integer
);

insert or ignore into utc_system (id, db_version) values (1, 1);

create table if not exists profile (
	id   integer primary key,
	name text
);

create table if not exists game (
	id          integer primary key,
	steam_appid integer unique, -- if -1, not a steam game
	name        text,
	allowed     integer,
	enabled     integer,

	profile_id  integer,

	date_touched  integer,

	foreign key (profile_id) references profile(id)
);

create table if not exists game_exe (
	id      integer primary key,
	game_id integer,
	path    text,

	foreign key (game_id) references game(id)
);




insert or ignore into profile (id, name) values (1, "Default");

-- insert or ignore into game (id, steam_appid, name, allowed, enabled, profile_id) values (1, 123, "Test Steam Game", 1, 1, 1);
insert or ignore into game (id, steam_appid, name, allowed, enabled, profile_id) values (2, -1, "Test non-Steam game", 1, 0, 1);
-- insert or ignore into game (id, steam_appid, name, allowed, enabled, profile_id) values (3, 456, "Popular Multiplayer Game", 0, 1, 1);


insert or ignore into game_exe (game_id, path) values (2, "C:\WINDOWS\system32\notepad.exe");
insert or ignore into game_exe (game_id, path) values (2, "C:\Windows\SysWOW64\notepad.exe");
insert or ignore into game_exe (game_id, path) values (2, "C:\Code\universal_time_control\dummy_win64.exe");
-- insert or ignore into game_exe (id, path) values (3, "C:\Code\gltest2\test_monolithic.exe");
insert or ignore into game_exe (game_id, path) values (2, "C:\Code\gltest2\test.exe");
-- insert or ignore into game_exe (id, path) values (4, "E:\do_not_backup\SteamLibrarySSD\steamapps\common\Dishonored\Binaries\Win32\Dishonored.exe");




create table if not exists control (
	id integer primary key,

	activation_mode   integer default 2, -- 0=press, 1=hold, 2=toggle

	key__v_code       integer default 114,
	key__scan_code    integer default 61,
	key__desc_utf8    text    default "CTRL+F3",
	key__ctrl         integer default 1,
	key__alt          integer default 0,
	key__shift        integer default 0,

	slow_or_fast                integer default 0, -- 0 = slow, 1 = fast
	slowdown_or_speedup_percent integer default 50,
	duration                    integer default 10, -- seconds
	limit_mode                  integer default 0, -- 0 = no limit, 1 = cooldown, 2 = energy

	cooldown_secs               integer default 5,

	max_energy                  integer default 100,
	cost_per_use                integer default 25,
	recharge_rate               integer default 5,
	min_energy_to_activate		integer default 20

);

insert into control (id) values (1);