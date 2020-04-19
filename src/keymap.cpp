#include "keymap.h"

#include <iostream>
#include <vector>

#include "config.h"
#include "confighandlerexception.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

struct OpDesc {
	Operation op;
	const char* opstr;
	const char* default_key;
	const char* help_text;
	unsigned short flags;
};

/*
 * This is the list of operations, defining operation, operation name (for
 * keybindings), default key, description, and where it's valid
 */
static OpDesc opdescs[] = {
	{
		OP_OPEN,
		"open",
		"ENTER",
		_("Open feed/article"),
		KM_FEEDLIST | KM_FILEBROWSER | KM_ARTICLELIST | KM_TAGSELECT |
		KM_FILTERSELECT | KM_URLVIEW | KM_DIALOGS | KM_DIRBROWSER
	},
	{
		OP_SWITCH_FOCUS,
		"switch-focus",
		"TAB",
		_("Switch focus between widgets"),
		KM_FILEBROWSER | KM_DIRBROWSER
	},
	{OP_QUIT, "quit", "q", _("Return to previous dialog/Quit"), KM_BOTH},
	{
		OP_HARDQUIT,
		"hard-quit",
		"Q",
		_("Quit program, no confirmation"),
		KM_BOTH
	},
	{
		OP_RELOAD,
		"reload",
		"r",
		_("Reload currently selected feed"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{OP_RELOADALL, "reload-all", "R", _("Reload all feeds"), KM_FEEDLIST},
	{
		OP_MARKFEEDREAD,
		"mark-feed-read",
		"A",
		_("Mark feed read"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_MARKALLFEEDSREAD,
		"mark-all-feeds-read",
		"C",
		_("Mark all feeds read"),
		KM_FEEDLIST
	},
	{
		OP_MARKALLABOVEASREAD,
		"mark-all-above-as-read",
		"",
		_("Mark all above as read"),
		KM_ARTICLELIST
	},
	{OP_SAVE, "save", "s", _("Save article"), KM_ARTICLELIST | KM_ARTICLE},
	{OP_SAVEALL, "save-all", "", _("Save articles"), KM_ARTICLELIST},
	{
		OP_NEXT,
		"next",
		"J",
		_("Go to next article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_PREV,
		"prev",
		"K",
		_("Go to previous article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_NEXTUNREAD,
		"next-unread",
		"n",
		_("Go to next unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_PREVUNREAD,
		"prev-unread",
		"p",
		_("Go to previous unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_RANDOMUNREAD,
		"random-unread",
		"^K",
		_("Go to a random unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_OPENBROWSER_AND_MARK,
		"open-in-browser-and-mark-read",
		"O",
		_("Open article in browser and mark read"),
		KM_ARTICLELIST
	},
	{
		OP_OPENALLUNREADINBROWSER,
		"open-all-unread-in-browser",
		"",
		_("Open all unread items of selected feed in browser"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_OPENALLUNREADINBROWSER_AND_MARK,
		"open-all-unread-in-browser-and-mark-read",
		"",
		_("Open all unread items of selected feed in browser and mark "
			"read"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_OPENINBROWSER,
		"open-in-browser",
		"o",
		_("Open article in browser"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_HELP,
		"help",
		"?",
		_("Open help dialog"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_PODBOAT
	},
	{
		OP_TOGGLESOURCEVIEW,
		"toggle-source-view",
		"^U",
		_("Toggle source view"),
		KM_ARTICLE
	},
	{
		OP_TOGGLEITEMREAD,
		"toggle-article-read",
		"N",
		_("Toggle read status for article"),
		KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_TOGGLESHOWREAD,
		"toggle-show-read-feeds",
		"l",
		_("Toggle show read feeds/articles"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_SHOWURLS,
		"show-urls",
		"u",
		_("Show URLs in current article"),
		KM_ARTICLE | KM_ARTICLELIST
	},
	{OP_CLEARTAG, "clear-tag", "^T", _("Clear current tag"), KM_FEEDLIST},
	{OP_SETTAG, "set-tag", "t", _("Select tag"), KM_FEEDLIST},
	{OP_SETTAG, "select-tag", "t", _("Select tag"), KM_FEEDLIST},
	{
		OP_SEARCH,
		"open-search",
		"/",
		_("Open search dialog"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_ARTICLE
	},
	{OP_GOTO_URL, "goto-url", "#", _("Goto URL #"), KM_ARTICLE},
	{OP_ENQUEUE, "enqueue", "e", _("Add download to queue"), KM_ARTICLE},
	{
		OP_RELOADURLS,
		"reload-urls",
		"^R",
		_("Reload the list of URLs from the configuration"),
		KM_FEEDLIST
	},
	{OP_PB_DOWNLOAD, "pb-download", "d", _("Download file"), KM_PODBOAT},
	{OP_PB_CANCEL, "pb-cancel", "c", _("Cancel download"), KM_PODBOAT},
	{
		OP_PB_DELETE,
		"pb-delete",
		"D",
		_("Mark download as deleted"),
		KM_PODBOAT
	},
	{
		OP_PB_PURGE,
		"pb-purge",
		"P",
		_("Purge finished and deleted downloads from queue"),
		KM_PODBOAT
	},
	{
		OP_PB_TOGGLE_DLALL,
		"pb-toggle-download-all",
		"a",
		_("Toggle automatic download on/off"),
		KM_PODBOAT
	},
	{
		OP_PB_PLAY,
		"pb-play",
		"p",
		_("Start player with currently selected download"),
		KM_PODBOAT
	},
	{
		OP_PB_MARK_FINISHED,
		"pb-mark-as-finished",
		"m",
		_("Mark file as finished (not played)"),
		KM_PODBOAT
	},
	{
		OP_PB_MOREDL,
		"pb-increase-max-dls",
		"+",
		_("Increase the number of concurrent downloads"),
		KM_PODBOAT
	},
	{
		OP_PB_LESSDL,
		"pb-decreate-max-dls",
		"-",
		_("Decrease the number of concurrent downloads"),
		KM_PODBOAT
	},
	{OP_REDRAW, "redraw", "^L", _("Redraw screen"), KM_SYSKEYS},
	{OP_CMDLINE, "cmdline", ":", _("Open the commandline"), KM_NEWSBOAT},
	{
		OP_SETFILTER,
		"set-filter",
		"F",
		_("Set a filter"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_SELECTFILTER,
		"select-filter",
		"f",
		_("Select a predefined filter"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_CLEARFILTER,
		"clear-filter",
		"^F",
		_("Clear currently set filter"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST
	},
	{
		OP_BOOKMARK,
		"bookmark",
		"^B",
		_("Bookmark current link/article"),
		KM_ARTICLELIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_EDITFLAGS,
		"edit-flags",
		"^E",
		_("Edit flags"),
		KM_ARTICLELIST | KM_ARTICLE
	},
	{OP_NEXTFEED, "next-feed", "j", _("Go to next feed"), KM_ARTICLELIST},
	{
		OP_PREVFEED,
		"prev-feed",
		"k",
		_("Go to previous feed"),
		KM_ARTICLELIST
	},
	{
		OP_NEXTUNREADFEED,
		"next-unread-feed",
		"^N",
		_("Go to next unread feed"),
		KM_ARTICLELIST
	},
	{
		OP_PREVUNREADFEED,
		"prev-unread-feed",
		"^P",
		_("Go to previous unread feed"),
		KM_ARTICLELIST
	},
	{OP_MACROPREFIX, "macro-prefix", ",", _("Call a macro"), KM_NEWSBOAT},
	{
		OP_DELETE,
		"delete-article",
		"D",
		_("Delete article"),
		KM_ARTICLELIST | KM_ARTICLE
	},
	{
		OP_DELETE_ALL,
		"delete-all-articles",
		"^D",
		_("Delete all articles"),
		KM_ARTICLELIST
	},
	{
		OP_PURGE_DELETED,
		"purge-deleted",
		"$",
		_("Purge deleted articles"),
		KM_ARTICLELIST
	},
	{
		OP_EDIT_URLS,
		"edit-urls",
		"E",
		_("Edit subscribed URLs"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_CLOSEDIALOG,
		"close-dialog",
		"^X",
		_("Close currently selected dialog"),
		KM_DIALOGS
	},
	{
		OP_VIEWDIALOGS,
		"view-dialogs",
		"v",
		_("View list of open dialogs"),
		KM_NEWSBOAT
	},
	{
		OP_NEXTDIALOG,
		"next-dialog",
		"^V",
		_("Go to next dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PREVDIALOG,
		"prev-dialog",
		"^G",
		_("Go to previous dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PIPE_TO,
		"pipe-to",
		"|",
		_("Pipe article to command"),
		KM_ARTICLE | KM_ARTICLELIST
	},
	{
		OP_SORT,
		"sort",
		"g",
		_("Sort current list"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_REVSORT,
		"rev-sort",
		"G",
		_("Sort current list (reverse)"),
		KM_FEEDLIST | KM_ARTICLELIST
	},

	{OP_0, "zero", "0", _("Open URL 10"), KM_URLVIEW | KM_ARTICLE},
	{OP_1, "one", "1", _("Open URL 1"), KM_URLVIEW | KM_ARTICLE},
	{OP_2, "two", "2", _("Open URL 2"), KM_URLVIEW | KM_ARTICLE},
	{OP_3, "three", "3", _("Open URL 3"), KM_URLVIEW | KM_ARTICLE},
	{OP_4, "four", "4", _("Open URL 4"), KM_URLVIEW | KM_ARTICLE},
	{OP_5, "five", "5", _("Open URL 5"), KM_URLVIEW | KM_ARTICLE},
	{OP_6, "six", "6", _("Open URL 6"), KM_URLVIEW | KM_ARTICLE},
	{OP_7, "seven", "7", _("Open URL 7"), KM_URLVIEW | KM_ARTICLE},
	{OP_8, "eight", "8", _("Open URL 8"), KM_URLVIEW | KM_ARTICLE},
	{OP_9, "nine", "9", _("Open URL 9"), KM_URLVIEW | KM_ARTICLE},

	{OP_SK_UP, "up", "UP", _("Move to the previous entry"), KM_SYSKEYS},
	{OP_SK_DOWN, "down", "DOWN", _("Move to the next entry"), KM_SYSKEYS},
	{
		OP_SK_PGUP,
		"pageup",
		"PAGEUP",
		_("Move to the previous page"),
		KM_SYSKEYS
	},
	{
		OP_SK_PGDOWN,
		"pagedown",
		"PAGEDOWN",
		_("Move to the next page"),
		KM_SYSKEYS
	},

	{
		OP_SK_HOME,
		"home",
		"HOME",
		_("Move to the start of page/list"),
		KM_SYSKEYS
	},
	{
		OP_SK_END,
		"end",
		"END",
		_("Move to the end of page/list"),
		KM_SYSKEYS
	},

	{
		OP_INT_END_QUESTION,
		"XXXNOKEY-end-question",
		"end-question",
		nullptr,
		KM_INTERNAL
	},
	{
		OP_INT_CANCEL_QNA,
		"XXXNOKEY-cancel-qna",
		"cancel-qna",
		nullptr,
		KM_INTERNAL
	},
	{
		OP_INT_QNA_NEXTHIST,
		"XXXNOKEY-qna-next-history",
		"qna-next-history",
		nullptr,
		KM_INTERNAL
	},
	{
		OP_INT_QNA_PREVHIST,
		"XXXNOKEY-qna-prev-history",
		"qna-prev-history",
		nullptr,
		KM_INTERNAL
	},

	{OP_INT_RESIZE, "RESIZE", "internal-resize", nullptr, KM_INTERNAL},
	{OP_INT_SET, "set", "internal-set", nullptr, KM_INTERNAL},

	{OP_INT_GOTO_URL, "gotourl", "internal-goto-url", nullptr, KM_INTERNAL},

	{OP_NIL, nullptr, nullptr, nullptr, 0}
};

// "all" must be first, the following positions must be the same as the KM_*
// flag definitions (get_flag_from_context() relies on this).
static const char* contexts[] = {"all",
		"feedlist",
		"filebrowser",
		"help",
		"articlelist",
		"article",
		"tagselection",
		"filterselection",
		"urlview",
		"podboat",
		"dialogs",
		"dirbrowser",
		nullptr
	};

KeyMap::KeyMap(unsigned flags)
{
	/*
	 * At startup, initialize the keymap with the default settings from the
	 * list above.
	 */
	LOG(Level::DEBUG, "KeyMap::KeyMap: flags = %x", flags);
	for (int i = 0; opdescs[i].op != OP_NIL; ++i) {
		const OpDesc op_desc = opdescs[i];
		if (!(op_desc.flags & (flags | KM_INTERNAL | KM_SYSKEYS))) {
			continue;
		}

		for (unsigned int j = 1; contexts[j] != nullptr; j++) {
			const std::string context(contexts[j]);
			const uint32_t context_flag = (1 << (j - 1));
			if ((op_desc.flags & (context_flag | KM_INTERNAL | KM_SYSKEYS))) {
				keymap_[context][op_desc.default_key] = op_desc.op;
			}
		}
	}
}

void KeyMap::get_keymap_descriptions(std::vector<KeyMapDesc>& descs,
	unsigned short flags)
{
	/*
	 * Here we return the keymap descriptions for the specified application
	 * (handed to us via flags) This is used for the help screen.
	 */
	for (unsigned int i = 1; contexts[i] != nullptr; i++) {
		std::string ctx(contexts[i]);

		if (flags & KM_PODBOAT && ctx != "podboat") {
			continue;
		} else if (flags & KM_NEWSBOAT && ctx == "podboat") {
			continue;
		}

		for (unsigned int j = 0; opdescs[j].op != OP_NIL; ++j) {
			bool already_added = false;
			for (const auto& keymap : keymap_[ctx]) {
				Operation op = keymap.second;
				if (op != OP_NIL) {
					if (opdescs[j].op == op &&
						opdescs[j].flags & flags) {
						KeyMapDesc desc;
						desc.key = keymap.first;
						desc.ctx = ctx;
						if (!already_added) {
							desc.cmd =
								opdescs[j]
								.opstr;
							if (opdescs[j].help_text)
								desc.desc = gettext(
										opdescs[j]
										.help_text);
							already_added = true;
						}
						desc.flags = opdescs[j].flags;
						descs.push_back(desc);
					}
				}
			}
			if (!already_added) {
				if (opdescs[j].flags & flags) {
					LOG(Level::DEBUG,
						"KeyMap::get_keymap_"
						"descriptions: "
						"found unbound function: %s "
						"ctx = "
						"%s",
						opdescs[j].opstr,
						ctx);
					KeyMapDesc desc;
					desc.ctx = ctx;
					desc.cmd = opdescs[j].opstr;
					if (opdescs[j].help_text)
						desc.desc = gettext(
								opdescs[j].help_text);
					desc.flags = opdescs[j].flags;
					descs.push_back(desc);
				}
			}
		}
	}
}

KeyMap::~KeyMap() {}

void KeyMap::set_key(Operation op,
	const std::string& key,
	const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::set_key(%d,%s) called", op, key);
	if (context == "all") {
		for (unsigned int i = 0; contexts[i] != nullptr; i++) {
			keymap_[contexts[i]][key] = op;
		}
	} else {
		keymap_[context][key] = op;
	}
}

void KeyMap::unset_key(const std::string& key, const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_key(%s) called", key);
	if (context == "all") {
		for (unsigned int i = 0; contexts[i] != nullptr; i++) {
			keymap_[contexts[i]][key] = OP_NIL;
		}
	} else {
		keymap_[context][key] = OP_NIL;
	}
}

void KeyMap::unset_all_keys(const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_all_keys(%s) called", context);
	auto internal_ops_only = get_internal_operations();
	if (context == "all") {
		for (unsigned int i = 0; contexts[i] != nullptr; i++) {
			keymap_[contexts[i]] = internal_ops_only;
		}
	} else {
		keymap_[context] = std::move(internal_ops_only);
	}
}

Operation KeyMap::get_opcode(const std::string& opstr)
{
	for (int i = 0; opdescs[i].opstr; ++i) {
		if (opstr == opdescs[i].opstr) {
			return opdescs[i].op;
		}
	}
	return OP_NIL;
}

char KeyMap::get_key(const std::string& keycode)
{
	if (keycode == "ENTER") {
		return '\n';
	} else if (keycode == "ESC") {
		return 27;
	} else if (keycode.length() == 2 && keycode[0] == '^') {
		char chr = keycode[1];
		return chr - '@';
	} else if (keycode.length() == 1) { // TODO: implement more keys
		return keycode[0];
	}
	return 0;
}

Operation KeyMap::get_operation(const std::string& keycode,
	const std::string& context)
{
	std::string key;
	LOG(Level::DEBUG,
		"KeyMap::get_operation: keycode = %s context = %s",
		keycode,
		context);
	if (keycode.length() > 0) {
		key = keycode;
	} else {
		key = "NIL";
	}
	return keymap_[context][key];
}

void KeyMap::dump_config(std::vector<std::string>& config_output)
{
	for (unsigned int i = 1; contexts[i] != nullptr;
		i++) { // TODO: optimize
		std::map<std::string, Operation>& x = keymap_[contexts[i]];
		for (const auto& keymap : x) {
			if (keymap.second < OP_INT_MIN) {
				std::string configline = "bind-key ";
				configline.append(utils::quote(keymap.first));
				configline.append(" ");
				configline.append(getopname(keymap.second));
				configline.append(" ");
				configline.append(contexts[i]);
				config_output.push_back(configline);
			}
		}
	}
	for (const auto& macro : macros_) {
		std::string configline = "macro ";
		configline.append(macro.first);
		configline.append(" ");
		unsigned int i = 0;
		for (const auto& cmd : macro.second) {
			configline.append(getopname(cmd.op));
			for (const auto& arg : cmd.args) {
				configline.append(" ");
				configline.append(utils::quote(arg));
			}
			if (i < (macro.second.size() - 1)) {
				configline.append(" ; ");
			}
		}
		config_output.push_back(configline);
	}
}

std::string KeyMap::getopname(Operation op)
{
	for (unsigned int i = 0; opdescs[i].op != OP_NIL; i++) {
		if (opdescs[i].op == op) {
			return opdescs[i].opstr;
		}
	}
	return "<none>";
}

void KeyMap::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	/*
	 * The keymap acts as ConfigActionHandler so that all the key-related
	 * configuration is immediately handed to it.
	 */
	LOG(Level::DEBUG, "KeyMap::handle_action(%s, ...) called", action);
	if (action == "bind-key") {
		if (params.size() < 2)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		std::string context = "all";
		if (params.size() >= 3) {
			context = params[2];
		}
		if (!is_valid_context(context))
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid context"), context));
		Operation op = get_opcode(params[1]);
		if (op == OP_NIL) {
			throw ConfigHandlerException(
				strprintf::fmt(_("`%s' is not a valid "
						"key command"),
					params[1]));
		}
		set_key(op, params[0], context);
	} else if (action == "unbind-key") {
		if (params.size() < 1) {
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		std::string context = "all";
		if (params.size() >= 2) {
			context = params[1];
		}
		if (params[0] == "-a") {
			unset_all_keys(context);
		} else {
			unset_key(params[0], context);
		}
	} else if (action == "macro") {
		if (params.size() < 1)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		auto it = params.begin();
		std::string macrokey = *it;
		std::vector<MacroCmd> cmds;
		MacroCmd tmpcmd;
		tmpcmd.op = OP_NIL;
		bool first = true;
		++it;

		while (it != params.end()) {
			if (first && *it != ";") {
				tmpcmd.op = get_opcode(*it);
				LOG(Level::DEBUG,
					"KeyMap::handle_action: new operation "
					"`%s' "
					"(op = %u)",
					*it,
					tmpcmd.op);
				if (tmpcmd.op == OP_NIL)
					throw ConfigHandlerException(
						strprintf::fmt(
							_("`%s' is not a valid "
								"key command"),
							*it));
				first = false;
			} else {
				if (*it == ";") {
					if (tmpcmd.op != OP_NIL) {
						cmds.push_back(tmpcmd);
					}
					tmpcmd.op = OP_NIL;
					tmpcmd.args.clear();
					first = true;
				} else {
					LOG(Level::DEBUG,
						"KeyMap::handle_action: new "
						"parameter `%s' (op = %u)",
						*it,
						tmpcmd.op);
					tmpcmd.args.push_back(*it);
				}
			}
			++it;
		}
		if (tmpcmd.op != OP_NIL) {
			cmds.push_back(tmpcmd);
		}

		macros_[macrokey] = cmds;
	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_PARAMS);
}

std::vector<std::string> KeyMap::get_keys(Operation op,
	const std::string& context)
{
	std::vector<std::string> keys;
	for (const auto& keymap : keymap_[context]) {
		if (keymap.second == op) {
			keys.push_back(keymap.first);
		}
	}
	return keys;
}

std::vector<MacroCmd> KeyMap::get_macro(const std::string& key)
{
	for (const auto& macro : macros_) {
		if (macro.first == key) {
			return macro.second;
		}
	}
	std::vector<MacroCmd> dummyvector;
	return dummyvector;
}

bool KeyMap::is_valid_context(const std::string& context)
{
	for (unsigned int i = 0; contexts[i] != nullptr; i++) {
		if (context == contexts[i]) {
			return true;
		}
	}
	return false;
}

std::map<std::string, Operation> KeyMap::get_internal_operations() const
{
	std::map<std::string, Operation> internal_ops;
	for (int i = 0; opdescs[i].op != OP_NIL; ++i) {
		if (opdescs[i].flags & KM_INTERNAL) {
			internal_ops[opdescs[i].default_key] = opdescs[i].op;
		}
	}
	return internal_ops;
}

unsigned short KeyMap::get_flag_from_context(const std::string& context)
{
	for (unsigned int i = 1; contexts[i] != nullptr; i++) {
		if (context == contexts[i]) {
			return (1 << (i - 1)) | KM_SYSKEYS;
		}
	}
	return 0; // shouldn't happen
}

} // namespace newsboat
