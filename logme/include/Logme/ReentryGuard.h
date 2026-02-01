namespace Logme
{
  thread_local int DisplayDepth = 0;

  class DisplayReentryGuard
  {
    bool Active;

  public:
    DisplayReentryGuard() : Active(false)
    {
      if (DisplayDepth)
        return;

      ++DisplayDepth;
      Active = true;
    }

    ~DisplayReentryGuard()
    {
      if (Active)
      {
        --DisplayDepth;
      }
    }

    bool IsActive() const
    {
      return Active;
    }
  };
}
