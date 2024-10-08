#include "gui.hpp"
#include <nana/gui/filebox.hpp>

void GUI::gui_bottom::show_btncopy(bool show)
{
	plc.field_display("btncopy", show);
	plc.field_display("btncopy_spacer", show);
	plc.collocate();
}


void GUI::gui_bottom::show_btnfmt(bool show)
{
	plc.field_display("btn_ytfmtlist", show);
	plc.field_display("ytfm_spacer", show);
	plc.collocate();
	pgui->get_place().collocate();
	nana::api::update_window(*pgui);
}

fs::path GUI::gui_bottom::file_path()
{
	if(!printed_path.empty())
	{
		if(printed_path.extension().string() != "NA")
		{
			if(fs::exists(printed_path))
				return printed_path;
		}
		if(!merger_path.empty())
		{
			printed_path.replace_extension(merger_path.extension());
			if(fs::exists(printed_path))
				return printed_path;
			if(!download_path.empty())
			{
				printed_path.replace_extension(download_path.extension());
				if(fs::exists(printed_path))
					return printed_path;
			}
			if(vidinfo_contains("ext"))
			{
				std::string ext {vidinfo["ext"]};
				printed_path.replace_extension(ext);
				if(fs::exists(printed_path))
					return printed_path;
			}
		}
	}

	if(!merger_path.empty() && fs::exists(merger_path))
		return merger_path;
	if(!download_path.empty() && fs::exists(download_path))
		return download_path;

	fs::path file;
	if(vidinfo_contains("filename"))
		file = fs::u8path(vidinfo["filename"].get<std::string>());

	if(!file.empty())
	{
		file = outpath / file;
		if(!fs::exists(file))
		{
			if(cbargs.checked() && com_args.caption().find("-f ") != -1)
			{
				auto args {com_args.caption()};
				auto pos {args.find("-f ")};
				if(pos != -1)
				{
					pos += 1;
					while(pos < args.size() && isspace(args[++pos]));
					std::string num;
					while(pos < args.size())
					{
						char c {args[pos++]};
						if(isdigit(c))
							num += c;
						else break;
					}
					if(!num.empty())
					{
						for(auto &fmt : vidinfo["formats"])
						{
							std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
							if(format_id == num)
							{
								file.replace_extension(ext);
								break;
							}
						}
						if(!fs::exists(file))
							file.clear();
					}
					else file.clear();
				}
				else file.clear();
			}
			else if(cbmp3.checked())
			{
				file.replace_extension("mp3");
				if(!fs::exists(file))
					file.clear();
			}
			else if(use_strfmt)
			{
				using nana::to_utf8;
				std::string ext1, ext2, vcodec;
				for(auto &fmt : vidinfo["formats"])
				{
					std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
					if(ext1.empty() && format_id == to_utf8(fmt1))
					{
						ext1 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(fmt2.empty()) break;
					}
					else if(!fmt2.empty() && format_id == to_utf8(fmt2))
					{
						ext2 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(!ext1.empty()) break;
					}
				}
				if(ext2.empty())
					file.replace_extension(ext1);
				else
				{
					std::string ext;
					if(ext2 == "webm")
						ext = (ext1 == "webm" || ext1 == "weba") ? "webm" : "mkv";
					else if(ext1 == "webm" || ext1 == "weba")
						ext = vcodec.starts_with("av01") ? "webm" : "mkv";
					else ext = "mp4";
					file.replace_extension(ext);
				}
				if(!fs::exists(file))
					file.clear();
			}
			else file.clear();
		}
	}
	return file;
}


GUI::gui_bottom::gui_bottom(GUI &gui, bool visible)
{
	using namespace nana;

	auto &conf {GUI::conf};
	pgui = &gui;
	create(gui, visible);
	bgcolor(::widgets::theme::fmbg);

	auto prevbot {gui.bottoms.back()};
	if(prevbot)
		outpath = prevbot->outpath;
	else outpath = conf.outpath;

	plc.bind(*this);
	gpopt.create(*this, "Download options");
	gpopt.enabled(false);
	plcopt.bind(gpopt);
	tbrate.create(gpopt);
	prog.create(*this);
	separator.create(*this);
	separator.refresh_theme();
	expcol.create(*this);
	btn_ytfmtlist.create(*this, "Select formats");
	btndl.create(*this, "Start download");
	btncopy.create(*this, "Apply options to all queue items");
	btnerase.create(gpopt);
	btnq.create(*this, gui.queue_panel.visible() ? "Show output" : "Show queue");
	l_out.create(gpopt, "Download folder:");
	l_out.text_align(nana::align::left, nana::align_v::center);
	l_rate.create(gpopt, "Download rate limit:");
	l_rate.text_align(nana::align::left, nana::align_v::center);
	l_chap.create(gpopt, "Chapters:");
	l_outpath.create(gpopt, &outpath);
	com_rate.create(gpopt);
	com_chap.create(gpopt);
	com_args.create(gpopt);
	cbkeyframes.create(gpopt, "Force keyframes at cuts");
	cbmp3.create(gpopt, "Convert audio to MP3");
	cbsubs.create(gpopt, "Embed subtitles");
	cbthumb.create(gpopt, "Embed thumbnail");
	cbtime.create(gpopt, "File modification time = time of writing");
	cbargs.create(gpopt, "Custom arguments:");

	btnq.events().click([&]
	{
		if(btnq.caption().find("queue") != -1)
			gui.show_queue();
		else gui.show_output();
	});

	btncopy.events().click([&]
	{
		gui.bottoms.propagate_cb_options(*this);
		gui.bottoms.propagate_args_options(*this);
		gui.bottoms.propagate_misc_options(*this);
	});

	plc.div(R"(vert
			<prog weight=30> 
			<separator weight=3>
			<weight=20 <> <expcol weight=20>>
			<gpopt weight=220> <gpopt_spacer weight=20>
			<weight=35 <> <btn_ytfmtlist weight=190> <ytfmt_spacer weight=20> <btncopy weight=328> <btncopy_spacer weight=20> 
				<btnq weight=180> <weight=20> <btndl weight=200> <>>
		)");
	plc["prog"] << prog;
	plc["separator"] << separator;
	plc["expcol"] << expcol;
	plc["gpopt"] << gpopt;
	plc["btncopy"] << btncopy;
	plc["btn_ytfmtlist"] << btn_ytfmtlist;
	plc["btndl"] << btndl;
	plc["btnq"] << btnq;

	plc.field_display("prog", false);
	plc.field_display("separator", true);
	plc.field_display("btncopy", false);
	plc.field_display("btncopy_spacer", false);
	show_btnfmt(false);

	prog.amount(1000);
	prog.caption("");

	gpopt.size({10, 10}); // workaround for weird caption display bug

	gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbtime weight=304> <> <cbsubs weight=140>
			> <weight=20>
			<weight=25 <l_chap weight=67> <weight=10> <com_chap weight=65> <> <cbkeyframes weight=194> <> <cbthumb weight=152> <> <cbmp3 weight=181>>
			<weight=20> <weight=24 <cbargs weight=164> <weight=15> <com_args> <weight=10> <btnerase weight=24>>
		)");

	gpopt["l_out"] << l_out;
	gpopt["l_outpath"] << l_outpath;
	gpopt["l_rate"] << l_rate;
	gpopt["tbrate"] << tbrate;
	gpopt["com_rate"] << com_rate;
	gpopt["l_chap"] << l_chap;
	gpopt["cbkeyframes"] << cbkeyframes;
	gpopt["cbmp3"] << cbmp3;
	gpopt["com_chap"] << com_chap;
	gpopt["cbsubs"] << cbsubs;
	gpopt["cbthumb"] << cbthumb;
	gpopt["cbtime"] << cbtime;
	gpopt["cbargs"] << cbargs;
	gpopt["com_args"] << com_args;
	gpopt["btnerase"] << btnerase;

	const auto dpi {API::screen_dpi(true)};
	if(dpi >= 240)
	{
		btnerase.image(arr_erase48_png, sizeof arr_erase48_png);
		btnerase.image_disabled(arr_erase48_disabled_png, sizeof arr_erase48_disabled_png);
	}
	else if(dpi >= 192)
	{
		btnerase.image(arr_erase32_png, sizeof arr_erase32_png);
		btnerase.image_disabled(arr_erase32_disabled_png, sizeof arr_erase32_disabled_png);
	}
	else if(dpi > 96)
	{
		btnerase.image(arr_erase22_ico, sizeof arr_erase22_ico);
		btnerase.image_disabled(arr_erase22_disabled_ico, sizeof arr_erase22_disabled_ico);
	}
	else
	{
		btnerase.image(arr_erase16_ico, sizeof arr_erase16_ico);
		btnerase.image_disabled(arr_erase16_disabled_ico, sizeof arr_erase16_disabled_ico);
	}

	tbrate.multi_lines(false);
	tbrate.padding(0, 5, 0, 5);

	for(auto &str : conf.argsets)
		com_args.push_back(to_utf8(str));
	com_args.caption(conf.argset);
	com_args.editable(true);

	com_args.events().focus([&] (const arg_focus &arg)
	{
		if(!arg.getting)
		{
			conf.argset = com_args.caption();
			if(conf.common_dl_options)
			{
				for(auto &pbot : gui.bottoms)
				{
					auto &bot {*pbot.second};
					if(bot.handle() != *this)
						bot.com_args.caption(conf.argset);
				}
			}
		}
	});

	com_args.events().selected([&]
	{
		conf.argset = com_args.caption();
		if(conf.common_dl_options && api::focus_window() == com_args)
			gui.bottoms.propagate_args_options(*this);
	});

	com_args.events().text_changed([&]
	{
		auto idx {com_args.caption_index()};
		if(idx != -1) btnerase.enable(true);
		else btnerase.enable(false);
		if(conf.common_dl_options && api::focus_window() == com_args)
		{
			for(auto &pbot : gui.bottoms)
			{
				auto &bot {*pbot.second};
				if(bot.handle() != handle())
					bot.com_args.caption(com_args.caption());
			}
		}
	});

	btnerase.events().click([&]
	{
		auto idx {com_args.caption_index()};
		if(idx != -1)
		{
			com_args.erase(idx);
			com_args.caption("");
			conf.argsets.erase(conf.argsets.begin() + idx);
			auto size {conf.argsets.size()};
			if(size && idx == size) idx--;
			com_args.option(idx);
			if(conf.common_dl_options)
				gui.bottoms.propagate_args_options(*this);
		}
	});

	if(prevbot)
		tbrate.caption(prevbot->tbrate.caption());
	else if(conf.ratelim)
		tbrate.caption(util::format_double(conf.ratelim, 1));

	tbrate.set_accept([this](wchar_t wc)->bool
	{
		const auto selpoints {tbrate.selection()};
		const auto selcount {selpoints.second.x - selpoints.first.x};
		return wc == keyboard::backspace || wc == keyboard::del || ((isdigit(wc) || wc == '.') && tbrate.text().size() < 5 || selcount);
	});

	tbrate.events().focus([this](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			auto str {tbrate.caption()};
			if(!str.empty() && str.back() == '.')
			{
				str.pop_back();
				tbrate.caption(str);
			}
			GUI::conf.ratelim = tbrate.to_double();
		}
	});

	tbrate.events().text_changed([&]
	{
		if(conf.common_dl_options && tbrate == api::focus_window())
			gui.bottoms.propagate_misc_options(*this);
	});

	com_chap.editable(false);
	com_chap.push_back(" ignore");
	com_chap.push_back(" embed");
	com_chap.push_back(" split");

	com_rate.editable(false);
	com_rate.push_back(" KB/s");
	com_rate.push_back(" MB/s");

	if(prevbot)
	{
		com_rate.option(prevbot->com_rate.option());
		com_chap.option(prevbot->com_chap.option());
	}
	else 
	{
		com_chap.option(conf.com_chap);
		com_rate.option(conf.ratelim_unit);
	}

	com_rate.events().selected([&]
	{
		conf.ratelim_unit = com_rate.option();
		if(conf.common_dl_options && com_rate == api::focus_window())
			gui.bottoms.propagate_misc_options(*this);
	});

	com_chap.events().selected([&]
	{
		conf.com_chap = com_chap.option();
		if(conf.common_dl_options)
			gui.bottoms.propagate_misc_options(*this);
	});

	expcol.events().click([&]
	{
		auto wdsz {api::window_size(gui)};
		const auto px {(nana::API::screen_dpi(true) >= 144) * gui.queue_panel.visible()};
		gui.change_field_attr(gui.get_place(), "Bottom", "weight", 325 - 240 * expcol.collapsed() - 27 * gui.queue_panel.visible() - px);
		for(auto &bottom : gui.bottoms)
		{
			auto &bot {*bottom.second};
			if(expcol.collapsed())
			{
				if(&bot == this)
				{
					if(!gui.no_draw_freeze)
					{
						gui.conf.gpopt_hidden = true;
						SendMessageA(gui.hwnd, WM_SETREDRAW, FALSE, 0);
					}
					auto dh {gui.dpi_scale(240)};
					wdsz.height -= dh;
					gui.minh -= dh;
					api::window_size(gui, wdsz);
				}
				else bot.expcol.operate(true);
				bot.plc.field_display("gpopt", false);
				bot.plc.field_display("gpopt_spacer", false);
				bot.plc.collocate();
				if(&bot == this && !gui.no_draw_freeze)
				{
					SendMessageA(gui.hwnd, WM_SETREDRAW, TRUE, 0);
					nana::api::refresh_window(gui);
				}
			}
			else
			{
				if(&bot == this)
				{
					if(!gui.no_draw_freeze)
					{
						gui.conf.gpopt_hidden = false;
						SendMessageA(gui.hwnd, WM_SETREDRAW, FALSE, 0);
					}
					auto dh {gui.dpi_scale(240)};
					wdsz.height += dh;
					gui.minh += dh;
					api::window_size(gui, wdsz);
				}
				else bot.expcol.operate(false);
				bot.plc.field_display("gpopt", true);
				bot.plc.field_display("gpopt_spacer", true);
				bot.plc.collocate();
				if(&bot == this && !gui.no_draw_freeze)
				{
					SendMessageA(gui.hwnd, WM_SETREDRAW, TRUE, 0);
					nana::api::refresh_window(gui);
				}
			}
		}
	});

	com_chap.tooltip("<bold>--embed-chapters</>\tAdd chapter markers to the video file.\n"
		"<bold>--split-chapters</> \t\tSplit video into multiple files based on chapters.");
	cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
		"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\n"
		"around the cuts. (<bold>--force-keyframes-at-cuts</>)");
	cbtime.tooltip("Do not use the Last-modified header to set the file modification time (<bold>--no-mtime</>)");
	cbsubs.tooltip("Embed subtitles in the video (only for mp4, webm and mkv videos) (<bold>--embed-subs</>)");
	cbthumb.tooltip("Embed thumbnail in the video as cover art (<bold>--embed-thumbnail</>)");
	cbmp3.tooltip("Convert the source audio to MPEG Layer 3 format and save it to an .mp3 file.\n"
		"The video is discarded if present, so it's preferable to download an audio-only\n"
		"format if one is available. (<bold>-x --audio-format mp3</>)\n\n"
		"To download the best audio-only format available, use the custom\nargument <bold>-f ba</>");
	btnerase.tooltip("Remove this argument set from the list");
	btncopy.tooltip("Copy the options for this queue item to all the other queue items.");
	btn_ytfmtlist.tooltip("Choose formats manually, instead of letting yt-dlp\nchoose automatically. "
		"By default, yt-dlp chooses the\nbest formats, according to the preferences you set\n"
		"(if you press the \"Settings\" button, you can set\nthe preferred resolution, container, and framerate).");
	std::string args_tip
	{
		"Custom options for yt-dlp, separated by space. Some useful examples:\n\n"
			"<bold>-f ba</> : best audio (downloads just audio-only format, if available)\n"
			"<bold>-f wa</> : worst audio\n"
			"<bold>-f wv+wa</> : combines the worst video format with the worst audio format\n"
			"<bold>--live-from-start</> : Downloads livestreams from the start. Currently \n"
			"								  only supported for YouTube (experimental).\n\n"
			"For more, read the yt-dlp documentation:\n"
			"https://github.com/yt-dlp/yt-dlp#usage-and-options"
	};
	cbargs.tooltip(args_tip);
	com_args.tooltip(args_tip);

	btndl.events().click([&gui, this] { gui.on_btn_dl(url); });

	btndl.events().key_press([&gui, this](const arg_keyboard &arg)
	{
		if(arg.key == '\r')
			gui.on_btn_dl(url);
	});

	btn_ytfmtlist.events().click([&gui, this]
	{
		gui.fm_formats();
	});

	l_outpath.events().click([&gui, &conf, this]
	{
		auto clip_text = [this](const std::wstring &str, int max_pixels) -> std::wstring
		{
			nana::label l {*this, str};
			l.typeface(l_outpath.typeface());
			int offset {0};
			const auto strsize {str.size()};
			while(l.measure(1234).width > max_pixels)
			{
				offset += 1;
				if(strsize - offset < 4)
					return L"";
				l.caption(L"..." + str.substr(offset));
			}
			return l.caption_wstring();
		};

		auto pop_folder_selection_box = [&gui, &conf, this]
		{
			nana::folderbox fb {*this, conf.open_dialog_origin ? outpath : gui.appdir};
			fb.allow_multi_select(false);
			fb.title("Locate and select the desired download folder");
			auto res {fb()};
			if(res.size())
			{
				conf.outpaths.insert(outpath);
				outpath = conf.outpath = res.front();
				l_outpath.caption(outpath.u8string());
				if(conf.common_dl_options)
					gui.bottoms.propagate_misc_options(*this);
			}
		};

		if(conf.outpaths.empty() || conf.outpaths.size() == 1 && *conf.outpaths.begin() == outpath)
		{
			pop_folder_selection_box();
			conf.outpaths.insert(outpath);
		}
		else
		{
			::widgets::Menu m;
			m.append("Choose folder...", [&, this](menu::item_proxy &)
			{
				pop_folder_selection_box();
				if(conf.outpaths.size() >= 11 && conf.outpaths.find(outpath) == conf.outpaths.end())
					conf.outpaths.erase(conf.outpaths.begin());
				conf.outpaths.insert(outpath);
			});
			m.append("Clear folder history", [&, this](menu::item_proxy &)
			{
				conf.outpaths.clear();
				conf.outpaths.insert(outpath);
			});
			m.append_splitter();
			for(auto &path : conf.outpaths)
			{
				if(path != outpath)
				{
					m.append(to_utf8(clip_text(path, gui.dpi_scale(250))), [&, this](menu::item_proxy &)
					{
						outpath = conf.outpath = path;
						l_outpath.update_caption();
						conf.outpaths.insert(outpath);
						if(conf.common_dl_options)
							gui.bottoms.propagate_misc_options(*this);
					});
				}
			}
			m.max_pixels(gui.dpi_scale(280));
			m.item_pixels(24);
			auto curpos {api::cursor_position()};
			m.popup_await(nullptr, curpos.x - 142, curpos.y);
			gui.queue_panel.focus();
		}
	});

	if(prevbot)
	{
		cbsubs.check(prevbot->cbsubs.checked());
		cbthumb.check(prevbot->cbthumb.checked());
		cbtime.check(prevbot->cbtime.checked());
		cbkeyframes.check(prevbot->cbkeyframes.checked());
		cbmp3.check(prevbot->cbmp3.checked());
		cbargs.check(prevbot->cbargs.checked());
	}
	else
	{
		cbsubs.check(conf.cbsubs);
		cbthumb.check(conf.cbthumb);
		cbtime.check(conf.cbtime);
		cbkeyframes.check(conf.cbkeyframes);
		cbmp3.check(conf.cbmp3);
		cbargs.check(conf.cbargs);
	}

	cbtime.events().checked([&]
	{
		conf.cbtime = cbtime.checked();
		if(conf.common_dl_options && cbtime == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbthumb.events().checked([&]
	{
		conf.cbthumb = cbthumb.checked();
		if(conf.common_dl_options && cbthumb == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbsubs.events().checked([&]
	{
		conf.cbsubs = cbsubs.checked();
		if(conf.common_dl_options && cbsubs == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbkeyframes.events().checked([&]
	{
		conf.cbkeyframes = cbkeyframes.checked();
		if(conf.common_dl_options && cbkeyframes == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbmp3.events().checked([&]
	{
		conf.cbmp3 = cbmp3.checked();
		if(conf.common_dl_options && cbmp3 == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbargs.events().checked([&]
	{
		conf.cbargs = cbargs.checked();
		if(conf.common_dl_options && cbargs == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});

	if(conf.common_dl_options && gui.bottoms.size())
		com_args.caption(gui.bottoms.at(0).com_args.caption());

	com_args.events().mouse_up([this](const arg_mouse &arg)
	{
		if(arg.button == mouse::right_button)
		{
			using namespace nana;
			::widgets::Menu m;
			m.item_pixels(24);

			auto cliptext {util::get_clipboard_text()};

			m.append("Paste", [&](menu::item_proxy)
			{
				com_args.focus();
				keybd_event(VK_LCONTROL, 0, 0, 0);
				keybd_event('V', 0, 0, 0);
				keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
				keybd_event(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
			}).enabled(!cliptext.empty());

			m.popup_await(com_args, arg.pos.x, arg.pos.y);
		}
	});

	gui.queue_panel.focus();
	plc.collocate();
}


bool GUI::gui_bottom::browse_for_filename()
{
	nana::filebox fb {*pgui, false};
	fb.allow_multi_select(false);
	fb.title("Specify the name to give the downloaded file");
	if(outfile.empty())
	{
		if(!vidinfo.empty() && vidinfo_contains("_filename"))
		{
			std::string fname {vidinfo["_filename"]};
			if(vidinfo_contains("requested_formats") && vidinfo["requested_formats"].size() > 1)
			{
				auto pos {fname.rfind('.')};
				if(pos != -1)
					fname = fname.substr(0, pos);
			}
			fb.init_file(outpath / fname);
		}
		else fb.init_file(outpath / "type the file name here (this overrides the output template from settings)");
	}
	else fb.init_file(outpath / outfile.filename());
	auto res {fb()};
	if(res.size())
	{
		outfile = res.front();
		outpath = conf.outpath = outfile.parent_path();
		l_outpath.caption(outpath.u8string());
		l_outpath.tooltip("Custom file name:\n<bold>" + outfile.filename().string() +
			"</>\n(this overrides the output template from the settings)");
		if(conf.common_dl_options)
			pgui->bottoms.propagate_misc_options(*this);
		if(conf.outpaths.size() >= 11 && conf.outpaths.find(outpath) == conf.outpaths.end())
			conf.outpaths.erase(conf.outpaths.begin());
		conf.outpaths.insert(outpath);
		return true;
	}
	return false;
}


void GUI::gui_bottom::apply_playsel_string()
{
	auto &str {playsel_string};
	playlist_selection.assign(playlist_info["entries"].size(), false);

	auto pos0 {str.rfind('|')};
	if(pos0 != -1)
		str.erase(0, pos0 + 1);

	int a {0}, b {0};
	size_t pos {0}, pos1 {0};
	pos1 = str.find_first_of(L",:", pos);
	if(pos1 == -1)
		playlist_selection[stoi(str) - 1] = true;
	while(pos1 != -1)
	{
		a = stoi(str.substr(pos, pos1 - pos)) - 1;
		pos = pos1 + 1;
		if(str[pos1] == ',')
		{
			playlist_selection[a] = true;
			pos1 = str.find_first_of(L",:", pos);
			if(pos1 == -1)
				playlist_selection[stoi(str.substr(pos)) - 1] = true;
		}
		else // ':'
		{
			pos1 = str.find(',', pos);
			if(pos1 != -1)
			{
				b = stoi(str.substr(pos, pos1 - pos)) - 1;
				pos = pos1 + 1;
				pos1 = str.find_first_of(L",:", pos);
				if(pos1 == -1)
					playlist_selection[stoi(str.substr(pos)) - 1] = true;
			}
			else b = stoi(str.substr(pos)) - 1;

			for(auto n {a}; n <= b; n++)
				playlist_selection[n] = true;
		}
	}
}


int GUI::gui_bottom::playlist_selected()
{
	int cnt {0};
	for(auto el : playlist_selection)
		if(el) cnt++;
	return cnt;
}


bool GUI::gui_bottom::vidinfo_contains(std::string key)
{
	if(vidinfo.contains(key) && vidinfo[key] != nullptr)
		return true;
	return false;
}


void GUI::gui_bottom::from_json(const nlohmann::json &j)
{
	using nana::to_wstring;

	if(j.contains("vidinfo"))
	{
		vidinfo = j["vidinfo"];
		show_btnfmt(true);
	}
	if(j.contains("playlist_info"))
	{
		playlist_info = j["playlist_info"];
		if(j.contains("playsel_string"))
		{
			playsel_string = to_wstring(j["playsel_string"].get<std::string>());
			apply_playsel_string();
		}
		else playlist_selection.assign(playlist_info["entries"].size(), true);
	}
	if(j.contains("strfmt"))
	{
		strfmt = to_wstring(j["strfmt"].get<std::string>());
		use_strfmt = j["use_strfmt"];
	}
	if(j.contains("fmt1"))
		fmt1 = to_wstring(j["fmt1"].get<std::string>());
	if(j.contains("fmt2"))
		fmt2 = to_wstring(j["fmt2"].get<std::string>());
	if(j.contains("cmdinfo"))
		cmdinfo = to_wstring(j["cmdinfo"].get<std::string>());
	if(j.contains("playlist_vid_cmdinfo"))
		playlist_vid_cmdinfo = to_wstring(j["playlist_vid_cmdinfo"].get<std::string>());
	if(j.contains("idx_error"))
		idx_error = j["idx_error"].get<int>();
	if(j.contains("is_yttab"))
		is_yttab = j["is_yttab"];
	if(j.contains("is_ytplaylist"))
		is_ytplaylist = j["is_ytplaylist"];
	if(j.contains("idx_error"))
		idx_error = j["idx_error"];
	if(j.contains("output_buffer"))
		pgui->outbox.buffer(url, j["output_buffer"].get<std::string>());
	if(j.contains("dlcmd"))
		pgui->outbox.commands[url] = j["dlcmd"].get<std::string>();
	if(j.contains("media_title"))
		media_title = j["media_title"].get<std::string>();
	if(j.contains("outfile"))
	{
		outfile = fs::u8path(j["outfile"].get<std::string>());
		outpath = conf.outpath = outfile.parent_path();
		l_outpath.caption(outpath.u8string());
		l_outpath.tooltip("Custom file name:\n<bold>" + outfile.filename().string() +
			"</>\n(this overrides the output template from the settings)");
	}
	if(j.contains("outpath"))
	{
		outpath = fs::u8path(j["outpath"].get<std::string>());
		l_outpath.caption(outpath.u8string());
		tbrate.caption(j["tbrate"].get<std::string>());
		com_rate.option(j["com_rate"]);
		cbtime.check(j["cbtime"]);
		cbsubs.check(j["cbsubs"]);
		com_chap.option(j["com_chap"]);
		cbkeyframes.check(j["cbkeyframes"]);
		cbthumb.check(j["cbthumb"]);
		cbmp3.check(j["cbmp3"]);
		cbargs.check(j["cbargs"]);
		com_args.caption(j["com_args"].get<std::string>());
	}
}


void GUI::gui_bottom::to_json(nlohmann::json &j)
{
	using namespace nana;
	if(!vidinfo.empty())
		j["vidinfo"] = vidinfo;
	if(!playlist_info.empty())
		j["playlist_info"] = playlist_info;
	if(!strfmt.empty())
	{
		j["strfmt"] = to_utf8(strfmt);
		j["use_strfmt"] = use_strfmt;
	}
	if(!fmt1.empty())
		j["fmt1"] = to_utf8(fmt1);
	if(!fmt2.empty())
		j["fmt2"] = to_utf8(fmt2);
	if(!cmdinfo.empty())
		j["cmdinfo"] = to_utf8(cmdinfo);
	if(!playlist_vid_cmdinfo.empty())
		j["playlist_vid_cmdinfo"] = to_utf8(playlist_vid_cmdinfo);
	if(!playsel_string.empty())
		j["playsel_string"] = to_utf8(playsel_string);
	if(idx_error)
		j["idx_error"] = idx_error;
	if(is_yttab)
		j["is_yttab"] = true;
	if(is_ytplaylist)
		j["is_ytplaylist"] = true;
	if(idx_error)
		j["idx_error"] = idx_error;
	if(!pgui->outbox.buffer(url).empty())
		j["output_buffer"] = pgui->outbox.buffer(url);
	if(pgui->outbox.commands.contains(url))
		j["dlcmd"] = pgui->outbox.commands[url];
	if(!outfile.empty())
		j["outfile"] = outfile.string();
	if(!media_title.empty())
		j["media_title"] = media_title;
	if(!conf.common_dl_options)
	{
		j["outpath"] = outpath.string();
		j["tbrate"] = tbrate.caption();
		j["com_rate"] = com_rate.option();
		j["cbtime"] = cbtime.checked();
		j["cbsubs"] = cbsubs.checked();
		j["com_chap"] = com_chap.option();
		j["cbkeyframes"] = cbkeyframes.checked();
		j["cbthumb"] = cbthumb.checked();
		j["cbmp3"] = cbmp3.checked();
		j["cbargs"] = cbargs.checked();
		j["com_args"] = com_args.caption();
	}
}