#include <Logme/Backend/SharedFileBackend.h>
#include <Logme/Channel.h>
#include <Logme/File/exe_path.h>
#include <Logme/Logger.h>
#include <Logme/Template.h>

using namespace Logme;

SharedFileBackend::SharedFileBackend(ChannelPtr owner)
  : Backend(owner, TYPE_ID)
  , MaxSize(MAX_SIZE_DEFAULT)
  , Timeout(10)
{
}

SharedFileBackend::~SharedFileBackend()
{
}

void SharedFileBackend::SetMaxSize(size_t size)
{
  MaxSize = size;
}

void SharedFileBackend::Display(Context& context, const char* line)
{

  if (CreateLog(NameTemplate.c_str()))
  {
    int nc;
    const char* buffer = context.Apply(Owner, Owner->GetFlags(), line, nc);
    Write(buffer, nc);

    CloseLog();
  }
}

std::string SharedFileBackend::GetPathName(int index)
{
  if (index < 0 || index > 1)
    return "";

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(Name.c_str(), param);
  std::string pathName = name;

  if (!IsAbsolutePath(pathName))
    pathName = Owner->GetOwner()->GetHomeDirectory() + name;

  return pathName;
}

std::string SharedFileBackend::FormatDetails()
{
  return NameTemplate;
}

BackendConfigPtr SharedFileBackend::CreateConfig()
{
  return std::make_shared<SharedFileBackendConfig>();
}

bool SharedFileBackend::ApplyConfig(BackendConfigPtr c)
{
  SharedFileBackendConfig* p = (SharedFileBackendConfig*)c.get();
  
  MaxSize = p->MaxSize;
  Timeout = p->Timeout;
  NameTemplate = p->Filename;

  return true;
}

bool SharedFileBackend::CreateLog(const char* v)
{
  NameTemplate = v;

  ProcessTemplateParam param;
  std::string name = ProcessTemplate(v, param);

  if (!IsAbsolutePath(name))
    name = Owner->GetOwner()->GetHomeDirectory() + name;

  CloseLog();
  Name.clear();

  Name = name;

  auto dir = std::filesystem::path(Name).parent_path();

  if (std::filesystem::exists(dir) == false)
    std::filesystem::create_directories(dir);

  return Open(true, unsigned(Timeout));
}

void SharedFileBackend::CloseLog()
{
  Close();
}
