#@  {
#@  "targets":
#@      [{
#@		 "name":"projectinfo.hpp"
#@		,"dependencies":[{"ref":"externals.json","rel":"misc"}
#@			,{"ref":"maikeconfig.json","rel":"misc"}
#@			,{"ref":"projectinfo.json","rel":"misc"}]
#@		,"status_check":"dynamic"
#@		}]
#@  }

import sys
import json
import string
import time
import subprocess

def write_error(*args, **kwargs):
    print(*args,file=sys.stderr,**kwargs)

projinfo_template=string.Template('''/*Information about $projname
*
* This file has been generated by $srcfile on $date.
* Any changes to this file may be overwritten during compilation.
*/

#ifndef ANJA_PROJECTINFO_HPP
#define ANJA_PROJECTINFO_HPP

namespace Anja
	{
	struct ProjectInfo
		{
		public:
			constexpr const char* name() const noexcept
				{return s_projname;}

			constexpr const char* revision() const noexcept
				{return s_revision;}

			constexpr const char* nameAndRevision() const noexcept
				{return s_name_rev;}

			constexpr const char* description() const noexcept
				{return s_description;}

			constexpr const char* author() const noexcept
				{return s_author;}

			constexpr const char* years() const noexcept
				{return s_years;}

			constexpr const char* copyright() const noexcept
				{return s_copyright;}

			constexpr const char* const* acknowledgement() const noexcept
				{return s_acknowledgement;}

			constexpr const char* acknowledgementAll() const noexcept
				{return s_acknowledgement_all;}

			constexpr const char* legalBrief() const noexcept
				{return s_legal_brief;}

			constexpr const char* const* libraries() const noexcept
				{return s_libraries;}

			constexpr const char* const* tools() const noexcept
				{return s_tools;}

			constexpr const char* compilationDate() const noexcept
				{return s_compilation_date;}

			constexpr const char* compiler() const noexcept
				{return s_compiler;}

			constexpr const char* architecture() const noexcept
				{return s_architecture;}

			constexpr const char* techstring() const noexcept
				{return s_techstring;}

		private:
			static constexpr const char* s_projname="$projname";
			static constexpr const char* s_revision="$revision";
			static constexpr const char* s_name_rev="$projname, $revision";
			static constexpr const char* s_description="$description";
			static constexpr const char* s_author="$author";
			static constexpr const char* s_years="$years";
			static constexpr const char* s_copyright="©\xa0$years\xa0$author";
			static constexpr const char* s_acknowledgement[]={"$acknowledgement",nullptr};
			static constexpr const char* s_acknowledgement_all="$acknowledgement_all";
			static constexpr const char* s_legal_brief="$legal_brief";
			static constexpr const char* s_libraries[]={"$libraries",nullptr};
			static constexpr const char* s_tools[]={"$tools",nullptr};
			static constexpr const char* s_compilation_date="$date";
			static constexpr const char* s_compiler="$compiler";
			static constexpr const char* s_architecture="$architecture";
			static constexpr const char* s_techstring="This $projname was "
				"compiled for $architecture by\\n$compiler, on $date, using $libstring.";
		};
	}

#endif
''')

def load(filename):
	with open(filename,encoding='utf-8') as input:
		return json.load(input)

def compiler_name(config):
	for hook in config['target_hooks']:
		if hook['name']=='targetcxx_default':
			return hook['config']['objcompile']['name']

def compiler_version(exename):
	compiler=subprocess.Popen([exename,'--version'] \
		,stdout=subprocess.PIPE)
	for lines in compiler.stdout:
		return lines.decode('utf8').rstrip()

def get_revision():
	with subprocess.Popen(('git', 'describe','--tags','--dirty','--always')\
		,stdout=subprocess.PIPE) as git:
		result=git.stdout.read().decode().strip()
		git.wait()
		status=git.returncode

	if status:
		with open('versioninfo.txt') as versionfile:
			result=versionfile.read().strip()
	else:
		with open('versioninfo.txt','w') as versionfile:
			versionfile.write(result)

	return result

try:
	target_dir=sys.argv[1]
	in_dir=sys.argv[2]
	substitutes=dict()
	substitutes['srcfile']=sys.argv[0]
	substitutes['date']=time.strftime('%Y-%m-%d %H:%M %Z')

	projinfo=load(in_dir + '/projectinfo.json')
	substitutes['projname']=projinfo['name']
	substitutes['acknowledgement']='","'.join(projinfo['acknowledgement'])
	substitutes['acknowledgement_all']='\\n'.join(projinfo['acknowledgement'])
	substitutes['author']=projinfo['author']
	substitutes['legal_brief']=projinfo['legal_brief']
	substitutes['years']=str(projinfo['years']).replace(', ','–').strip('[]')
	substitutes['revision']=get_revision()
	substitutes['description']=projinfo['description']

	externals=load(target_dir + '/externals.json')
	substitutes['libraries']='","'.join(externals['libraries'])
	substitutes['libstring']=', '.join(externals['libraries'])
	substitutes['tools']='","'.join(externals['tools'])

	config=load(target_dir + '/maikeconfig.json')
	substitutes['compiler']=compiler_version(compiler_name(config)).replace(' ','\xa0')
	substitutes['architecture']=config['targetinfo']['architecture']

	with open(target_dir + '/' + in_dir + '/projectinfo.hpp','wb') as output:
		output.write(projinfo_template.substitute(substitutes).encode('utf-8'))
	sys.exit(0)

except Exception:
	write_error('%s:%d: error: %s\n'%(sys.argv[0],sys.exc_info()[2].tb_lineno,sys.exc_info()[1]))
	sys.exit(-1)
