static string JS_modules =
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief JavaScript server functions\n"
  "///\n"
  "/// @file\n"
  "///\n"
  "/// DISCLAIMER\n"
  "///\n"
  "/// Copyright by triAGENS GmbH - All rights reserved.\n"
  "///\n"
  "/// The Programs (which include both the software and documentation)\n"
  "/// contain proprietary information of triAGENS GmbH; they are\n"
  "/// provided under a license agreement containing restrictions on use and\n"
  "/// disclosure and are also protected by copyright, patent and other\n"
  "/// intellectual and industrial property laws. Reverse engineering,\n"
  "/// disassembly or decompilation of the Programs, except to the extent\n"
  "/// required to obtain interoperability with other independently created\n"
  "/// software or as specified by law, is prohibited.\n"
  "///\n"
  "/// The Programs are not intended for use in any nuclear, aviation, mass\n"
  "/// transit, medical, or other inherently dangerous applications. It shall\n"
  "/// be the licensee's responsibility to take all appropriate fail-safe,\n"
  "/// backup, redundancy, and other measures to ensure the safe use of such\n"
  "/// applications if the Programs are used for such purposes, and triAGENS\n"
  "/// GmbH disclaims liability for any damages caused by such use of\n"
  "/// the Programs.\n"
  "///\n"
  "/// This software is the confidential and proprietary information of\n"
  "/// triAGENS GmbH. You shall not disclose such confidential and\n"
  "/// proprietary information and shall use it only in accordance with the\n"
  "/// terms of the license agreement you entered into with triAGENS GmbH.\n"
  "///\n"
  "/// Copyright holder is triAGENS GmbH, Cologne, Germany\n"
  "///\n"
  "/// @author Dr. Frank Celler\n"
  "/// @author Copyright 2011, triAGENS GmbH, Cologne, Germany\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @page JSModules JavaScript Modules\n"
  "///\n"
  "/// The AvocadoDB uses a <a\n"
  "/// href=\"http://wiki.commonjs.org/wiki/Modules\">CommonJS</a> compatible module\n"
  "/// concept. You can use the function @FN{require} in order to load a\n"
  "/// module. @FN{require} returns the exported variables and functions of the\n"
  "/// module. You can use the option @CO{startup.modules-path} to specify the\n"
  "/// location of the JavaScript files.\n"
  "///\n"
  "/// @section Modules Modules\n"
  "///\n"
  "/// @copydetails JSF_require\n"
  "///\n"
  "/// @section InternalFunctions Internal Functions\n"
  "///\n"
  "/// The following functions are used internally to implement the module loader.\n"
  "///\n"
  "/// @copydetails JS_Execute\n"
  "///\n"
  "/// @copydetails JS_Load\n"
  "///\n"
  "/// @copydetails JS_Read\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "// -----------------------------------------------------------------------------\n"
  "// --SECTION--                                                            Module\n"
  "// -----------------------------------------------------------------------------\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @addtogroup V8Module\n"
  "/// @{\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief module cache\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "ModuleCache = {};\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief module constructor\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "function Module (id) {\n"
  "  this.id = id;\n"
  "  this.exports = {};\n"
  "}\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief loads a file and creates a new module descriptor\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "Module.prototype.require = function (path) {\n"
  "  var content;\n"
  "  var sandbox;\n"
  "  var paths;\n"
  "  var module;\n"
  "\n"
  "  // first get rid of any \"..\" and \".\"\n"
  "  path = this.normalise(path);\n"
  "\n"
  "  // check if you already know the module, return the exports\n"
  "  if (path in ModuleCache) {\n"
  "    return ModuleCache[path].exports;\n"
  "  }\n"
  "\n"
  "  // try to load the file\n"
  "  paths = MODULES_PATH.split(\";\");\n"
  "  content = undefined;\n"
  "\n"
  "  for (var i = 0;  i < paths.length;  ++i) {\n"
  "    var p = paths[i];\n"
  "    var n;\n"
  "\n"
  "    if (p == \"\") {\n"
  "      n = \".\" + path + \".js\"\n"
  "    }\n"
  "    else {\n"
  "      n = p + \"/\" + path + \".js\";\n"
  "    }\n"
  "\n"
  "    if (FS_EXISTS(n)) {\n"
  "      content = read(n);\n"
  "      break;\n"
  "    }\n"
  "  }\n"
  "\n"
  "  if (content == undefined) {\n"
  "    throw \"cannot find a module named '\" + path + \"' using the module path(s) '\" + MODULES_PATH + \"'\";\n"
  "  }\n"
  "\n"
  "  // create a new sandbox and execute\n"
  "  ModuleCache[path] = module = new Module(path);\n"
  "\n"
  "  sandbox = {};\n"
  "  sandbox.module = module;\n"
  "  sandbox.exports = module.exports;\n"
  "  sandbox.require = function(path) { return sandbox.module.require(path); }\n"
  "  sandbox.print = print;\n"
  "\n"
  "  execute(content, sandbox, path);\n"
  "\n"
  "  module.exports = sandbox.exports;\n"
  "\n"
  "  return module.exports;\n"
  "};\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief normalises a path\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "Module.prototype.normalise = function (path) {\n"
  "  var p;\n"
  "  var q;\n"
  "  var x;\n"
  "\n"
  "  if (path == \"\") {\n"
  "    return this.id;\n"
  "  }\n"
  "\n"
  "  p = path.split('/');\n"
  "\n"
  "  // relative path\n"
  "  if (p[0] == \".\" || p[0] == \"..\") {\n"
  "    q = this.id.split('/');\n"
  "    q.pop();\n"
  "    q = q.concat(p);\n"
  "  }\n"
  "\n"
  "  // absolute path\n"
  "  else {\n"
  "    q = p;\n"
  "  }\n"
  "\n"
  "  // normalize path\n"
  "  n = [];\n"
  "\n"
  "  for (var i = 0;  i < q.length;  ++i) {\n"
  "    x = q[i];\n"
  "\n"
  "    if (x == \"\") {\n"
  "    }\n"
  "    else if (x == \".\") {\n"
  "    }\n"
  "    else if (x == \"..\") {\n"
  "      if (n.length == 0) {\n"
  "        throw \"cannot cross module top\";\n"
  "      }\n"
  "\n"
  "      n.pop();\n"
  "    }\n"
  "    else {\n"
  "      n.push(x);\n"
  "    }\n"
  "  }\n"
  "\n"
  "  return \"/\" + n.join('/');\n"
  "};\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief top-level model\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "ModuleCache[\"/\"] = module = new Module(\"/\");\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief top-level require\n"
  "///\n"
  "/// @FUN{require(@FA{path})}\n"
  "///\n"
  "/// Require checks if the file specified by @FA{path} has already been loaded.\n"
  "/// If not, the content of the file is executed in a new context. Within the\n"
  "/// context you can use the global variable @CODE{exports} in order to export\n"
  "/// variables and functions. This variable is returned by @FN{require}. \n"
  "///\n"
  "/// Assume that your module file is @CODE{test1.js} and contains\n"
  "///\n"
  "/// @verbinclude modules1\n"
  "///\n"
  "/// Then you can use require to load the file and access the exports.\n"
  "///\n"
  "/// @verbinclude modules2\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "function require (path) {\n"
  "  return module.require(path);\n"
  "}\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @}\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "// -----------------------------------------------------------------------------\n"
  "// --SECTION--                                                            Module\n"
  "// -----------------------------------------------------------------------------\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @addtogroup V8ModuleFS\n"
  "/// @{\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @brief fs model\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "ModuleCache[\"/fs\"] = new Module(\"/fs\");\n"
  "ModuleCache[\"/fs\"].exports.exists = FS_EXISTS;\n"
  "\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "/// @}\n"
  "////////////////////////////////////////////////////////////////////////////////\n"
  "\n"
  "// Local Variables:\n"
  "// mode: outline-minor\n"
  "// outline-regexp: \"^\\\\(/// @brief\\\\|/// @addtogroup\\\\|// --SECTION--\\\\)\"\n"
  "// End:\n"
;
