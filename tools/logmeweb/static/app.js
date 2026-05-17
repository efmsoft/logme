let CurrentTarget = null;
let CurrentView = 'overview';
let LastDiscovery = [];
let CachedChannels = null;

function $(id)
{
  return document.getElementById(id);
}

function EscapeHtml(value)
{
  return String(value ?? '')
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#039;');
}

function SetScreen(name)
{
  $('connectScreen').classList.toggle('active', name === 'connect');
  $('mainScreen').classList.toggle('active', name === 'main');
}

function NormalizeTarget(item, password)
{
  const control = item.control || {};

  return {
    pid: item.pid || null,
    process: item.process || 'Manual target',
    instance: item.instance || '',
    version: item.version || '',
    logmeVersion: item.logmeVersion || '',
    protocolVersion: item.protocolVersion || item.version || '',
    host: control.host || item.host || '127.0.0.1',
    port: Number(control.port || item.port || 0),
    protocol: (control.protocol || item.protocol || 'http').toLowerCase(),
    authRequired: Boolean(item.authRequired),
    password: password || ''
  };
}

async function ConnectToTarget(item, password, statusElement = null)
{
  const target = NormalizeTarget(item, password);

  if (statusElement)
  {
    statusElement.classList.remove('error-inline');
    statusElement.classList.add('muted');
    statusElement.textContent = 'Connecting...';
  }

  try
  {
    const check = await SendCommandToTarget(target, 'overview', 'text');
    if (!check.ok)
      throw new Error(check.error || check.text || 'connection check failed');

    CurrentTarget = target;
    CurrentView = 'overview';
    CachedChannels = null;
    UpdateTargetSummary();
    SetScreen('main');
    LoadTargetVersion();
    await SelectView('overview');
  }
  catch (e)
  {
    if (statusElement)
    {
      statusElement.classList.remove('muted');
      statusElement.classList.add('error-inline');
      statusElement.textContent = `Connection failed: ${e.message || e}`;
    }
    else
    {
      alert(`Connection failed: ${e.message || e}`);
    }
  }
}

function ParseVersionText(text)
{
  const result = {};
  const lines = String(text || '').split(/\r?\n/);

  for (const line of lines)
  {
    const match = line.match(/^([^:]+):\s*(.*)$/);
    if (!match)
      continue;

    const key = match[1].trim().toLowerCase();
    const value = match[2].trim();

    if (key === 'logme version')
      result.logmeVersion = value;
    else if (key === 'control protocol')
      result.protocolVersion = value;
  }

  return result;
}

async function LoadTargetVersion()
{
  if (!CurrentTarget)
    return;

  try
  {
    const result = await SendCommand('version', 'text');
    if (!result.ok)
      return;

    const parsed = ParseVersionText(result.text);

    if (parsed.logmeVersion)
      CurrentTarget.logmeVersion = parsed.logmeVersion;

    if (parsed.protocolVersion)
      CurrentTarget.protocolVersion = parsed.protocolVersion;

    UpdateTargetSummary();
  }
  catch (e)
  {
  }
}

function UpdateTargetSummary()
{
  const target = CurrentTarget;

  $('targetSummary').innerHTML = `
    <div class="target-name">${EscapeHtml(target.process)}</div>
    <div class="target-line">${EscapeHtml(target.protocol.toUpperCase())} ${EscapeHtml(target.host)}:${EscapeHtml(target.port)}</div>
    <div class="target-line">PID: ${EscapeHtml(target.pid || 'n/a')}</div>
    <div class="target-line">Logme version: ${EscapeHtml(target.logmeVersion || 'n/a')}</div>
    <div class="target-line">Control protocol: ${EscapeHtml(target.protocolVersion || target.version || 'n/a')}</div>
  `;
}

async function LoadDiscovery()
{
  const status = $('discoveryStatus');
  const list = $('discoveryList');

  status.textContent = 'Searching...';
  list.innerHTML = '';

  try
  {
    const response = await fetch('/api/discovery');
    const data = await response.json();
    LastDiscovery = data;

    if (!data.length)
    {
      status.textContent = 'No discoverable logme applications found.';
      return;
    }

    status.textContent = `${data.length} application(s) found.`;

    for (const item of data)
    {
      const control = item.control || {};
      const div = document.createElement('div');
      div.className = 'discovery-item';

      div.innerHTML = `
        <div class="discovery-main">
          <div class="process-name">${EscapeHtml(item.process || 'Unknown process')}</div>
          <div class="process-meta">PID ${EscapeHtml(item.pid)} · ${EscapeHtml((control.protocol || 'http').toUpperCase())} ${EscapeHtml(control.host || '127.0.0.1')}:${EscapeHtml(control.port || '')}</div>
        </div>
        <div class="discovery-auth">
          <input type="password" placeholder="Password${item.authRequired ? '' : ' optional'}">
          <button class="primary">Connect</button>
        </div>
      `;

      div.querySelector('button').addEventListener('click', () =>
      {
        const password = div.querySelector('input').value;
        let status = div.querySelector('.connect-status');
        if (!status)
        {
          status = document.createElement('div');
          status.className = 'connect-status status-line muted';
          div.querySelector('.discovery-auth').appendChild(status);
        }
        ConnectToTarget(item, password, status);
      });

      list.appendChild(div);
    }
  }
  catch (e)
  {
    status.textContent = `Discovery failed: ${e}`;
  }
}

async function SendCommandToTarget(target, command, format = 'text')
{
  const response = await fetch('/api/command', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      host: target.host,
      port: target.port,
      protocol: target.protocol,
      password: target.password,
      command: command,
      format: format
    })
  });

  const data = await response.json();

  if (!response.ok)
    throw new Error(data.detail || 'request failed');

  return data;
}

async function SendCommand(command, format = 'text')
{
  if (!CurrentTarget)
    throw new Error('No target selected');

  return await SendCommandToTarget(CurrentTarget, command, format);
}

function RenderResponseCard(title, command, result)
{
  const okClass = result.ok ? 'ok' : 'bad';
  const error = result.error ? `<div class="error-text">${EscapeHtml(result.error)}</div>` : '';
  const text = result.text || '';

  return `
    <div class="result-card">
      <div class="result-header">
        <div>
          <div class="result-title">${EscapeHtml(title)}</div>
          <div class="command-line">${EscapeHtml(command)}</div>
        </div>
        <span class="badge ${okClass}">${result.ok ? 'OK' : 'ERROR'}</span>
      </div>
      ${error}
      <pre>${EscapeHtml(text)}</pre>
    </div>
  `;
}

function ChannelNameForDisplay(name)
{
  return name === '' ? '<default>' : name;
}

function QuoteArg(value)
{
  const text = String(value ?? '');
  if (/^[A-Za-z0-9_.:-]+$/.test(text))
    return text;

  return '"' + text.replaceAll('\\', '\\\\').replaceAll('"', '\\"') + '"';
}

function TracePointPatternArg(value)
{
  const text = String(value ?? '');
  if (!/[\s"]/.test(text))
    return text;

  return '*' + text.replaceAll('"', '?').replaceAll(/\s+/g, '*') + '*';
}

function ChannelControlNameArg(channel)
{
  if (!channel || channel === '<default>')
    return '<default>';

  return QuoteArg(channel);
}

function ChannelOptionArg(channel)
{
  if (!channel)
    return '';

  return ` --channel ${QuoteArg(channel)}`;
}

async function LoadChannels()
{
  if (CachedChannels)
    return CachedChannels;

  const result = await SendCommand('list', 'json');
  CachedChannels = result.json?.data?.channels || [];
  return CachedChannels;
}

async function LoadChannelDetail(channel)
{
  const command = channel ? `channel ${QuoteArg(channel)}` : 'channel';
  return await SendCommand(command, 'text');
}

function BuildChannelCommand(base, channel, tail = '')
{
  const channelArg = ChannelOptionArg(channel);
  const suffix = tail ? ` ${tail}` : '';
  return `${base}${channelArg}${suffix}`.trim();
}

function RenderTags(items, emptyText)
{
  if (!items.length)
    return `<div class="empty-state small">${EscapeHtml(emptyText)}</div>`;

  return `<div class="tag-list">${items.map(x => `<span class="tag">${EscapeHtml(x)}</span>`).join('')}</div>`;
}

function ParseBackendTypes(channelText)
{
  const result = {};
  const lines = (channelText || '').split(/\r?\n/);

  for (const line of lines)
  {
    const match = line.match(/^\s+([A-Za-z0-9_]+Backend)\b/);
    if (!match)
      continue;

    result[match[1]] = (result[match[1]] || 0) + 1;
  }

  return result;
}

function ParseAccessCount(channelText)
{
  const match = (channelText || '').match(/^Access count:\s*(\d+)/m);
  if (!match)
    return 0;

  return Number(match[1] || 0);
}

function ParseLoggedBytes(channelText)
{
  const match = (channelText || '').match(/^Logged bytes:\s*(\d+)/m);
  if (!match)
    return 0;

  return Number(match[1] || 0);
}

function FormatBytes(value)
{
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let index = 0;
  let n = Number(value || 0);

  while (n >= 1024 && index + 1 < units.length)
  {
    n /= 1024;
    ++index;
  }

  if (index === 0)
    return `${Math.round(n)} ${units[index]}`;

  const fixed = n >= 100 ? n.toFixed(0) : n >= 10 ? n.toFixed(1) : n.toFixed(2);
  return `${fixed} ${units[index]}`;
}

function FormatBytesWithExact(value)
{
  const bytes = Number(value || 0);
  const compact = FormatBytes(bytes);

  if (bytes === 0 || compact === `${bytes} B`)
    return compact;

  return `${compact} (${bytes} bytes)`;
}

function ParseKeyValueText(text)
{
  const result = {};
  const lines = (text || '').split(/\r?\n/);

  for (const line of lines)
  {
    const match = line.match(/^([^:]+):\s*(.*)$/);
    if (!match)
      continue;

    result[match[1].trim().toLowerCase()] = match[2].trim();
  }

  return result;
}

function ParseOverviewBackends(text)
{
  const result = [];
  const lines = (text || '').split(/\r?\n/);
  let active = false;

  for (const line of lines)
  {
    if (/^Backends:\s*none\s*$/i.test(line))
      return [];

    if (/^Backends:\s*$/i.test(line))
    {
      active = true;
      continue;
    }

    if (!active)
      continue;

    const match = line.match(/^\s+([^:]+):\s*(\d+)\s*$/);
    if (!match)
    {
      if (/^\S/.test(line))
        break;
      continue;
    }

    result.push({ name: match[1], count: Number(match[2] || 0) });
  }

  return result;
}

function NormalizeLevelName(value)
{
  const text = String(value || '').trim();
  if (!text)
    return 'UNKNOWN';

  return text.toUpperCase();
}


function MergeCounters(dst, src)
{
  for (const [key, value] of Object.entries(src))
    dst[key] = (dst[key] || 0) + value;
}

function RenderStatCard(label, value, note)
{
  return `
    <div class="stat-card">
      <div class="stat-label">${EscapeHtml(label)}</div>
      <div class="stat-value">${EscapeHtml(value)}</div>
      ${note ? `<div class="stat-note">${EscapeHtml(note)}</div>` : ''}
    </div>
  `;
}

async function RenderOverview()
{
  $('workArea').innerHTML = '<div class="loading">Loading overview...</div>';

  try
  {
    const result = await SendCommand('overview', 'text');
    const values = ParseKeyValueText(result.text || '');
    const backends = ParseOverviewBackends(result.text || '');

    const channels = values['channels'] || '0';
    const logCalls = values['log write calls'] || '0';
    const loggedBytes = Number(values['logged bytes'] || 0);
    const errorChannel = values['error channel'] || '<none>';
    const obfuscation = values['obfuscation'] || 'Disabled';

    const backendHtml = backends.length
      ? backends
        .sort((a, b) => a.name.localeCompare(b.name))
        .map(item => `<div class="metric-line"><span>${EscapeHtml(item.name)}</span><strong>${EscapeHtml(item.count)}</strong></div>`)
        .join('')
      : '<div class="empty-state small">No backends found</div>';

    $('workArea').innerHTML = `
      <div class="stats-grid overview-stats">
        ${RenderStatCard('Channels', channels, '')}
        ${RenderStatCard('Log write calls', logCalls, '')}
        ${RenderStatCard('Logged bytes', FormatBytesWithExact(loggedBytes), '')}
      </div>

      <div class="cards-grid two-columns">
        <div class="card soft">
          <h2>Runtime</h2>
          <div class="metric-list">
            <div class="metric-line"><span>Home directory</span><strong>${EscapeHtml(values['home directory'] || '')}</strong></div>
            <div class="metric-line"><span>Error channel</span><strong>${EscapeHtml(errorChannel)}</strong></div>
            <div class="metric-line"><span>Auto delete channels</span><strong>${EscapeHtml(values['auto delete channels'] || '0')}</strong></div>
            <div class="metric-line"><span>VT mode</span><strong>${EscapeHtml(values['vt mode requested'] || 'Disabled')}</strong></div>
            <div class="metric-line"><span>Obfuscation</span><strong>${EscapeHtml(obfuscation)}</strong></div>
            ${values['obfuscation key'] ? `<div class="metric-line"><span>Obfuscation key</span><strong class="mono-value">${EscapeHtml(values['obfuscation key'])}</strong></div>` : ''}
          </div>
        </div>

        <div class="card soft">
          <h2>Backend types</h2>
          <div class="metric-list">${backendHtml}</div>
        </div>
      </div>
    `;
  }
  catch (e)
  {
    $('workArea').innerHTML = `<div class="error-card">${EscapeHtml(e)}</div>`;
  }
}


async function ExecuteChannelAction(channel, command, resultId)
{
  const target = $(resultId);
  target.innerHTML = '<div class="loading">Sending command...</div>';

  try
  {
    const result = await SendCommand(command, 'text');
    target.innerHTML = RenderResponseCard('Command result', command, result);
    CachedChannels = null;

    if (CurrentView === 'channels' && result.ok)
    {
      if (/^channel\s+--delete\b/i.test(command))
        await RenderChannels();
      else
        await RenderChannelDetails(channel);
    }
  }
  catch (e)
  {
    target.innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
  }
}


function ParseChannelField(channelText, names)
{
  const lines = (channelText || '').split(/\r?\n/);

  for (const line of lines)
  {
    for (const name of names)
    {
      const re = new RegExp('^\\s*' + name + '\\s*[:=]\\s*(.+?)\\s*$', 'i');
      const match = line.match(re);
      if (match)
        return match[1];
    }
  }

  return '';
}

function ParseChannelEnabled(channelText)
{
  const value = ParseChannelField(channelText, ['status', 'enabled', 'enable', 'active']);
  if (!value)
    return 'unknown';

  if (/\benabled\b/i.test(value) || /^(1|true|yes|on)$/i.test(value))
    return 'enabled';

  if (/\bdisabled\b/i.test(value) || /^(0|false|no|off)$/i.test(value))
    return 'disabled';

  return value;
}


function ParseChannelLevel(channelText)
{
  return NormalizeLevelName(ParseChannelField(channelText, ['level', 'log level', 'current level']) || 'unknown');
}


function ParseBoundChannel(channelText)
{
  const value = ParseChannelField(channelText, ['linked to', 'bound channel', 'redirect channel', 'redirected to', 'attached channel', 'bind']);
  if (!value || value === '<none>')
    return null;

  if (value === '<default>')
    return '';

  return value;
}

function ParseErrorChannel(channelText)
{
  const value = ParseChannelField(channelText, ['error channel']);
  if (!value)
    return 'unknown';

  if (/^(yes|true|1)$/i.test(value))
    return 'yes';

  if (/^(no|false|0)$/i.test(value))
    return 'no';

  return value;
}

function ParseChannelFlags(channelText)
{
  const line = (channelText || '').split(/\r?\n/).find(item => /^\s*Flags:/i.test(item));
  if (!line)
    return { value: '', names: [] };

  const valueMatch = line.match(/Flags:\s*([^\s]+)/i);
  const namesMatch = line.match(/\[(.*)\]/);
  const names = namesMatch
    ? namesMatch[1].split(/\s+/).map(x => x.trim()).filter(Boolean)
    : [];

  return {
    value: valueMatch ? valueMatch[1] : '',
    names: names
  };
}

function FlagRemoveArgument(flag)
{
  const name = String(flag || '').split(':')[0].toLowerCase();
  if (name === 'timestamp' || name === 'location' || name === 'console')
    return `${name}=none`;

  if (name === 'threadtransition')
    return 'transition=false';

  return `${name}=false`;
}


function ParseChannelBackends(channelText)
{
  const backends = [];
  const lines = (channelText || '').split(/\r?\n/);

  for (const line of lines)
  {
    let match = line.match(/^\s*(?:[-*]\s*)?([A-Za-z0-9_]+Backend)\b\s*(.*)$/);
    if (!match)
      match = line.match(/^\s*backend\s*[:=]\s*([A-Za-z0-9_]+)\b\s*(.*)$/i);

    if (!match)
      continue;

    const details = match[2] || '';

    backends.push({
      type: match[1],
      details: details.replace('[frozen]', '').trim(),
      frozen: /\[frozen\]/i.test(details)
    });
  }

  return backends;
}



const FlagDefinitions = [
  { name: 'timestamp', kind: 'value', values: ['local', 'utc', 'none'] },
  { name: 'signature', kind: 'bool' },
  { name: 'method', kind: 'bool' },
  { name: 'eol', kind: 'bool' },
  { name: 'errorprefix', kind: 'bool' },
  { name: 'duration', kind: 'bool' },
  { name: 'threadid', kind: 'bool' },
  { name: 'processid', kind: 'bool' },
  { name: 'channel', kind: 'bool' },
  { name: 'highlight', kind: 'bool' },
  { name: 'disablelink', kind: 'bool' },
  { name: 'transition', kind: 'bool' },
  { name: 'location', kind: 'value', values: ['none', 'short', 'full'] },
  { name: 'console', kind: 'value', values: ['cout', 'warncerr', 'errcerr', 'cerrcerr', 'cerr'] },
  { name: 'format', kind: 'value', values: ['text', 'json', 'xml'] }
];

function GetFlagDefinition(name)
{
  const text = String(name || '').toLowerCase();
  return FlagDefinitions.find(item => item.name === text) || FlagDefinitions[0];
}

function BuildFlagValueControl(selectedName)
{
  const def = GetFlagDefinition(selectedName);

  if (def.kind === 'bool')
    return '';

  return `
    <select id="addFlagValue">
      ${def.values.map(value => `<option value="${EscapeHtml(value)}">${EscapeHtml(value)}</option>`).join('')}
    </select>
  `;
}

function BuildAddFlagArgument()
{
  const name = $('addFlagSelect').value;
  const def = GetFlagDefinition(name);

  if (def.kind === 'bool')
    return `${name}=true`;

  const value = $('addFlagValue') ? $('addFlagValue').value : '';
  return `${name}=${value}`;
}

function RefreshFlagValueControl()
{
  const holder = $('addFlagValueHolder');
  if (!holder)
    return;

  holder.innerHTML = BuildFlagValueControl($('addFlagSelect').value);
}

function RenderChannelProperties(channel, detailText, channels)
{
  const enabled = ParseChannelEnabled(detailText);
  const level = ParseChannelLevel(detailText);
  const accessCount = ParseAccessCount(detailText);
  const loggedBytes = ParseLoggedBytes(detailText);
  const boundChannel = ParseBoundChannel(detailText);
  const errorChannel = ParseErrorChannel(detailText);
  const flags = ParseChannelFlags(detailText);

  const bindCandidates = [];

  if (channel !== '')
    bindCandidates.push('');

  for (const item of channels)
  {
    if (item === channel)
      continue;

    if (item === '' && bindCandidates.includes(''))
      continue;

    bindCandidates.push(item);
  }

  const channelOptions = bindCandidates
    .map(item => `<option value="${EscapeHtml(item)}" ${item === boundChannel ? 'selected' : ''}>${EscapeHtml(ChannelNameForDisplay(item))}</option>`)
    .join('');

  const activeFlags = flags.names.length
    ? flags.names.map(flag => `
      <span class="flag-chip">
        ${EscapeHtml(flag)}
        <button title="Remove flag" data-flag-remove="${EscapeHtml(flag)}">×</button>
      </span>
    `).join('')
    : '<span class="inline-muted">No active flags reported</span>';

  return `
    <div class="channel-properties plain-properties">
      <div class="property-row">
        <div class="property-label">Level</div>
        <div class="property-value property-edit inline-edit">
          <strong class="current-value">${EscapeHtml(level)}</strong>
          <select id="channelLevelValue">
            <option value="debug">DEBUG</option>
            <option value="info">INFO</option>
            <option value="warn">WARN</option>
            <option value="error">ERROR</option>
            <option value="critical">CRITICAL</option>
          </select>
          <button class="secondary small-button" id="applyChannelLevelButton">Apply</button>
        </div>
      </div>

      <div class="property-row">
        <div class="property-label">Enabled</div>
        <div class="property-value property-edit inline-edit">
          <span class="status-text ${enabled === 'enabled' ? 'on' : enabled === 'disabled' ? 'off' : ''}">${EscapeHtml(enabled)}</span>
          <button class="secondary small-button" id="enableChannelButton">Enable</button>
          <button class="secondary small-button" id="disableChannelButton">Disable</button>
        </div>
      </div>

      <div class="property-row">
        <div class="property-label">Error channel</div>
        <div class="property-value property-edit inline-edit">
          ${errorChannel === 'yes' ? `
            <span class="flag-chip error-channel-chip">
              assigned
              <button title="Clear error channel" id="clearErrorChannelChipButton">×</button>
            </span>
          ` : `<span class="inline-muted">${EscapeHtml(errorChannel === 'no' ? 'not assigned' : errorChannel)}</span>`}
          <button class="secondary small-button" id="setErrorChannelButton">Set as error channel</button>
        </div>
      </div>

      <div class="property-row">
        <div class="property-label">Access count</div>
        <div class="property-value">${EscapeHtml(accessCount)}</div>
      </div>

      <div class="property-row">
        <div class="property-label">Logged bytes</div>
        <div class="property-value">${EscapeHtml(FormatBytesWithExact(loggedBytes))}</div>
      </div>

      <div class="property-row">
        <div class="property-label">Bound channel</div>
        <div class="property-value property-edit inline-edit">
          ${boundChannel !== null ? `
            <span class="flag-chip bound-channel-chip">
              ${EscapeHtml(ChannelNameForDisplay(boundChannel))}
              <button title="Unbind channel" id="unbindChannelChipButton">×</button>
            </span>
          ` : '<span class="inline-muted">&lt;none&gt;</span>'}
          <select id="bindChannelSelect">
            ${channelOptions}
          </select>
          <button class="secondary small-button" id="bindChannelButton">Bind</button>
        </div>
      </div>

      <div class="property-row tall-row">
        <div class="property-label">Flags</div>
        <div class="property-value">
          <div class="flag-value">${EscapeHtml(flags.value)}</div>
          <div class="flag-chip-list">${activeFlags}</div>
          <div class="property-edit inline-edit flag-add-row">
            <select id="addFlagSelect">
              ${FlagDefinitions.map(flag => `<option value="${EscapeHtml(flag.name)}">${EscapeHtml(flag.name)}</option>`).join('')}
            </select>
            <span id="addFlagValueHolder">${BuildFlagValueControl('timestamp')}</span>
            <button class="secondary small-button" id="addFlagButton">Set</button>
          </div>
        </div>
      </div>
    </div>
  `;
}


function RenderBackendList(channel, backends)
{
  if (!backends.length)
  {
    return `
      <div class="empty-state small">
        No backends were found in channel details.
      </div>
    `;
  }

  return `
    <div class="backend-list">
      ${backends.map((backend, index) => `
        <div class="backend-item ${backend.frozen ? 'frozen' : ''}">
          <div>
            <div class="backend-name">${EscapeHtml(backend.type)} ${backend.frozen ? '<span class="frozen-badge">frozen</span>' : ''}</div>
            <div class="backend-details">${EscapeHtml(backend.details || 'no additional details')}</div>
          </div>
          <button class="danger small-button" data-backend-delete="${EscapeHtml(index)}">Delete</button>
        </div>
      `).join('')}
    </div>
  `;
}


function BuildBackendAdvancedFields()
{
  const type = $('addBackendType') ? $('addBackendType').value : '';

  if (type === 'FileBackend')
  {
    return `
      <label>File name</label>
      <input id="addBackendFileName" placeholder="log file path">

      <label class="check-line">
        <input id="addBackendAppend" type="checkbox" checked>
        Append to existing file
      </label>

      <label>Max size</label>
      <input id="addBackendMaxSize" placeholder="8mb">

      <label class="check-line">
        <input id="addBackendDailyRotation" type="checkbox">
        Daily rotation
      </label>

      <label>Max parts</label>
      <input id="addBackendMaxParts" placeholder="2">
    `;
  }

  if (type === 'SharedFileBackend')
  {
    return `
      <label>File name</label>
      <input id="addBackendFileName" placeholder="log file path">

      <label>Max size</label>
      <input id="addBackendMaxSize" placeholder="8mb">

      <label>Timeout</label>
      <input id="addBackendTimeout" placeholder="10ms">
    `;
  }

  if (type === 'BufferBackend')
  {
    return `
      <label>Max size</label>
      <input id="addBackendMaxSize" placeholder="4mb">

      <label>Policy</label>
      <select id="addBackendPolicy">
        <option value="stop-appending">stop-appending</option>
        <option value="delete-oldest">delete-oldest</option>
      </select>
    `;
  }

  return '';
}

function BuildAddBackendCommand(channel)
{
  const type = $('addBackendType').value;
  const asyncMode = $('addBackendAsync').checked;
  let tail = `--add ${type}`;

  if (asyncMode && type === 'ConsoleBackend')
    tail += ' --async';

  if (type === 'FileBackend' || type === 'SharedFileBackend')
  {
    const fileName = $('addBackendFileName') ? $('addBackendFileName').value.trim() : '';
    const maxSize = $('addBackendMaxSize') ? $('addBackendMaxSize').value.trim() : '';

    if (fileName)
      tail += ` --file ${QuoteArg(fileName)}`;

    if (maxSize)
      tail += ` --max-size ${QuoteArg(maxSize)}`;
  }

  if (type === 'FileBackend')
  {
    const append = $('addBackendAppend') ? $('addBackendAppend').checked : false;
    const dailyRotation = $('addBackendDailyRotation') ? $('addBackendDailyRotation').checked : false;
    const maxParts = $('addBackendMaxParts') ? $('addBackendMaxParts').value.trim() : '';

    tail += append ? ' --append' : ' --overwrite';
    tail += dailyRotation ? ' --daily-rotation' : ' --no-daily-rotation';

    if (maxParts)
      tail += ` --max-parts ${QuoteArg(maxParts)}`;
  }

  if (type === 'SharedFileBackend')
  {
    const timeout = $('addBackendTimeout') ? $('addBackendTimeout').value.trim() : '';
    if (timeout)
      tail += ` --timeout ${QuoteArg(timeout)}`;
  }

  if (type === 'BufferBackend')
  {
    const maxSize = $('addBackendMaxSize') ? $('addBackendMaxSize').value.trim() : '';
    const policy = $('addBackendPolicy') ? $('addBackendPolicy').value : '';

    if (maxSize)
      tail += ` --max-size ${QuoteArg(maxSize)}`;

    if (policy)
      tail += ` --policy ${policy}`;
  }

  return BuildChannelCommand('backend', channel, tail);
}

async function RenderChannelDetails(channel)
{
  $('channelDetails').innerHTML = '<div class="loading">Loading channel details...</div>';

  try
  {
    const detail = await LoadChannelDetail(channel);
    const detailText = detail.text || '';
    const backends = ParseChannelBackends(detailText);
    const channelTitle = ChannelNameForDisplay(channel);
    const channels = await LoadChannels();

    $('channelDetails').innerHTML = `
      <div class="channel-detail-main">
        <div class="card soft">
          <div class="card-header compact-header">
            <div>
              <h2>${EscapeHtml(channelTitle)}</h2>
              <p>Channel state and channel-specific operations.</p>
            </div>
            <button class="danger" id="deleteChannelButton" ${channel ? '' : 'disabled'}>Delete channel</button>
          </div>

          ${RenderChannelProperties(channel, detailText, channels)}
        </div>

        <div class="card soft">
          <div class="card-header compact-header">
            <div>
              <h2>Backends</h2>
              <p>Backends belong to this channel. Add or remove them here.</p>
            </div>
            <button class="primary" id="showAddBackendButton">Add backend</button>
          </div>

          ${RenderBackendList(channel, backends)}
        </div>

        <div class="modal-backdrop" id="addBackendModal" style="display: none;">
          <div class="modal-card">
            <div class="card-header compact-header">
              <div>
                <h2>Add backend</h2>
                <p>Select backend type and parameters for ${EscapeHtml(channelTitle)}.</p>
              </div>
            </div>

            <div class="form-grid">
              <label>Backend type</label>
              <select id="addBackendType">
                <option value="ConsoleBackend">ConsoleBackend</option>
                <option value="DebugBackend">DebugBackend</option>
                <option value="FileBackend">FileBackend</option>
                <option value="SharedFileBackend">SharedFileBackend</option>
                <option value="BufferBackend">BufferBackend</option>
                <option value="RingBufferBackend">RingBufferBackend</option>
              </select>

              <label class="check-line">
                <input id="addBackendAsync" type="checkbox">
                Async backend
              </label>

              <div id="addBackendAdvanced"></div>

              <label>Generated command</label>
              <input id="addBackendCommand" readonly>
            </div>

            <div class="button-row modal-actions">
              <button class="primary" id="applyAddBackendButton">Add backend</button>
              <button class="secondary" id="cancelAddBackendButton">Cancel</button>
            </div>
          </div>
        </div>

        <details class="raw-details">
          <summary>Raw channel response</summary>
          ${RenderResponseCard(channelTitle, channel ? `channel ${QuoteArg(channel)}` : 'channel', detail)}
        </details>

        <div id="channelActionResult"></div>
      </div>
    `;

    $('enableChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --enable ${ChannelControlNameArg(channel)}`, 'channelActionResult'));
    $('disableChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --disable ${ChannelControlNameArg(channel)}`, 'channelActionResult'));
    $('deleteChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --delete ${QuoteArg(channel)}`, 'channelActionResult'));
    $('setErrorChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --error ${ChannelControlNameArg(channel)}`, 'channelActionResult'));
    if ($('clearErrorChannelChipButton'))
      $('clearErrorChannelChipButton').addEventListener('click', () => ExecuteChannelAction(channel, 'channel --clear-error', 'channelActionResult'));

    $('channelLevelValue').value = ParseChannelLevel(detailText).toLowerCase();
    $('applyChannelLevelButton').addEventListener('click', () =>
    {
      const value = $('channelLevelValue').value;
      ExecuteChannelAction(channel, BuildChannelCommand('level', channel, value), 'channelActionResult');
    });

    $('bindChannelButton').addEventListener('click', () =>
    {
      const target = $('bindChannelSelect').value;
      ExecuteChannelAction(channel, `channel --bind ${ChannelControlNameArg(channel)} ${ChannelControlNameArg(target)}`, 'channelActionResult');
    });

    if ($('unbindChannelChipButton'))
    {
      $('unbindChannelChipButton').addEventListener('click', () =>
      {
        ExecuteChannelAction(channel, `channel --unbind ${ChannelControlNameArg(channel)}`, 'channelActionResult');
      });
    }

    document.querySelectorAll('[data-flag-remove]').forEach(button =>
    {
      button.addEventListener('click', () =>
      {
        const arg = FlagRemoveArgument(button.dataset.flagRemove);
        ExecuteChannelAction(channel, BuildChannelCommand('flags', channel, arg), 'channelActionResult');
      });
    });

    $('addFlagSelect').addEventListener('change', RefreshFlagValueControl);

    $('addFlagButton').addEventListener('click', () =>
    {
      ExecuteChannelAction(channel, BuildChannelCommand('flags', channel, BuildAddFlagArgument()), 'channelActionResult');
    });

    document.querySelectorAll('[data-backend-delete]').forEach(button =>
    {
      button.addEventListener('click', () =>
      {
        const backend = backends[Number(button.dataset.backendDelete)];
        ExecuteChannelAction(channel, BuildChannelCommand('backend', channel, `--delete ${backend.type}`), 'channelActionResult');
      });
    });

    const updateAddBackendCommand = () =>
    {
      $('addBackendCommand').value = BuildAddBackendCommand(channel);
    };

    const refreshAddBackendAdvanced = () =>
    {
      $('addBackendAdvanced').innerHTML = BuildBackendAdvancedFields();

      ['addBackendFileName', 'addBackendAppend', 'addBackendMaxSize', 'addBackendDailyRotation', 'addBackendMaxParts', 'addBackendTimeout', 'addBackendPolicy'].forEach(id =>
      {
        const element = $(id);
        if (element)
          element.addEventListener('input', updateAddBackendCommand);
        if (element)
          element.addEventListener('change', updateAddBackendCommand);
      });

      updateAddBackendCommand();
    };

    const showModal = () =>
    {
      $('addBackendModal').style.display = 'flex';
      refreshAddBackendAdvanced();
    };

    const hideModal = () =>
    {
      $('addBackendModal').style.display = 'none';
    };

    $('showAddBackendButton').addEventListener('click', showModal);
    $('cancelAddBackendButton').addEventListener('click', hideModal);
    $('addBackendType').addEventListener('change', refreshAddBackendAdvanced);
    $('addBackendAsync').addEventListener('change', updateAddBackendCommand);
    $('applyAddBackendButton').addEventListener('click', () =>
    {
      hideModal();
      ExecuteChannelAction(channel, $('addBackendCommand').value, 'channelActionResult');
    });
  }
  catch (e)
  {
    $('channelDetails').innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
  }
}


async function RenderChannels()
{
  $('workArea').innerHTML = '<div class="loading">Loading channels...</div>';

  const channels = await LoadChannels();

  let html = `
    <div class="split-view channel-manager">
      <div class="list-panel">
        <div class="section-title">Channels</div>
        <div class="hint">Select a channel to inspect it and perform channel-specific operations.</div>
        <input id="channelFilter" class="filter-input" placeholder="Filter channels...">
        <div class="item-list" id="channelList">
  `;

  for (const channel of channels)
  {
    html += `
      <button class="list-item" data-channel="${EscapeHtml(channel)}">
        <span>${EscapeHtml(ChannelNameForDisplay(channel))}</span>
      </button>
    `;
  }

  html += `
        </div>

        <div class="create-box card soft compact-card">
          <h3>Create channel</h3>
          <input id="newChannelName" placeholder="new channel name">
          <button class="primary full" id="createChannelButton">Create</button>
        </div>
      </div>
      <div class="detail-panel" id="channelDetails">
        <div class="empty-state">Select a channel on the left.</div>
      </div>
    </div>
  `;

  $('workArea').innerHTML = html;

  $('channelFilter').addEventListener('input', () =>
  {
    const filter = $('channelFilter').value.toLowerCase();
    document.querySelectorAll('#channelList .list-item').forEach(button =>
    {
      const text = button.textContent.toLowerCase();
      button.style.display = text.includes(filter) ? '' : 'none';
    });
  });

  $('createChannelButton').addEventListener('click', async () =>
  {
    const name = $('newChannelName').value.trim();
    if (!name)
      return;

    $('channelDetails').innerHTML = '<div class="loading">Creating channel...</div>';
    await ExecuteChannelAction(name, `channel --create ${QuoteArg(name)}`, 'channelDetails');
    CachedChannels = null;
    await RenderChannels();
  });

  async function SelectChannelButton(button)
  {
    document.querySelectorAll('#channelList .list-item').forEach(x => x.classList.remove('selected'));
    button.classList.add('selected');
    await RenderChannelDetails(button.dataset.channel);
  }

  document.querySelectorAll('#channelList .list-item').forEach(button =>
  {
    button.addEventListener('click', async () =>
    {
      await SelectChannelButton(button);
    });
  });

  const defaultChannelButton = document.querySelector('#channelList .list-item[data-channel=""]');
  if (defaultChannelButton)
    await SelectChannelButton(defaultChannelButton);
}

function RenderSubsystemChip(name, listName)
{
  const command = listName === 'blocked'
    ? `subsystem --unblock ${QuoteArg(name)}`
    : `subsystem --disallow ${QuoteArg(name)}`;

  return `
    <span class="flag-chip subsystem-chip">
      ${EscapeHtml(name)}
      <button title="Remove from ${EscapeHtml(listName)} list" data-subsystem-command="${EscapeHtml(command)}">×</button>
    </span>
  `;
}

function RenderSubsystemList(items, listName, emptyText)
{
  if (!items.length)
    return `<div class="empty-state small">${EscapeHtml(emptyText)}</div>`;

  return `<div class="flag-chip-list subsystem-chip-list">${items.map(item => RenderSubsystemChip(item, listName)).join('')}</div>`;
}

async function ExecuteSubsystemCommand(command)
{
  const resultBox = $('subsystemActionResult');
  resultBox.innerHTML = '<div class="loading">Sending command...</div>';

  try
  {
    const result = await SendCommand(command, 'text');
    resultBox.innerHTML = RenderResponseCard('Subsystem command result', command, result);

    if (result.ok)
      await RenderSubsystems();
  }
  catch (e)
  {
    resultBox.innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
  }
}

async function RenderSubsystems()
{
  $('workArea').innerHTML = '<div class="loading">Loading subsystems...</div>';

  const result = await SendCommand('subsystem', 'json');
  const data = result.json?.data || {};
  const blocked = data.blockedSubsystems || [];
  const allowed = data.allowedSubsystems || [];

  $('workArea').innerHTML = `
    <div class="subsystem-layout">
      <div class="card soft">
        <div class="card-header compact-header">
          <div>
            <h2>Subsystem filters</h2>
            <p>Add or remove subsystem names from the runtime block and allow lists.</p>
          </div>
        </div>

        <div class="subsystem-add-grid">
          <label>List</label>
          <select id="subsystemListKind">
            <option value="block">Block list</option>
            <option value="allow">Allow list</option>
          </select>

          <label>Subsystem name</label>
          <input id="subsystemNameInput" placeholder="subsystem name, max 8 chars">

          <button class="primary" id="addSubsystemButton">Add subsystem</button>
        </div>
      </div>

      <div class="cards-grid two-columns">
        <div class="card soft">
          <div class="card-header compact-header">
            <div>
              <h2>Blocked subsystems</h2>
              <p>Subsystems explicitly blocked at runtime.</p>
            </div>
            <button class="secondary small-button" id="clearBlockedSubsystemsButton">Clear</button>
          </div>
          ${RenderSubsystemList(blocked, 'blocked', 'No blocked subsystems')}
        </div>

        <div class="card soft">
          <div class="card-header compact-header">
            <div>
              <h2>Allowed subsystems</h2>
              <p>Subsystems explicitly allowed at runtime.</p>
            </div>
            <button class="secondary small-button" id="clearAllowedSubsystemsButton">Clear</button>
          </div>
          ${RenderSubsystemList(allowed, 'allowed', 'No allowed subsystems')}
        </div>
      </div>
      <div id="subsystemActionResult"></div>

      <details class="raw-details">
        <summary>Raw subsystem response</summary>
        ${RenderResponseCard('Raw response', 'subsystem', result)}
      </details>
    </div>
  `;

  $('addSubsystemButton').addEventListener('click', () =>
  {
    const name = $('subsystemNameInput').value.trim();
    if (!name)
      return;

    const op = $('subsystemListKind').value === 'block' ? '--block' : '--allow';
    ExecuteSubsystemCommand(`subsystem ${op} ${QuoteArg(name)}`);
  });

  $('clearBlockedSubsystemsButton').addEventListener('click', () => ExecuteSubsystemCommand('subsystem --clear-blocked'));
  $('clearAllowedSubsystemsButton').addEventListener('click', () => ExecuteSubsystemCommand('subsystem --clear-allowed'));
  document.querySelectorAll('[data-subsystem-command]').forEach(button =>
  {
    button.addEventListener('click', () => ExecuteSubsystemCommand(button.dataset.subsystemCommand));
  });
}

function ParseTracePointLine(line)
{
  const text = String(line || '').trim();
  if (!text || text === 'none')
    return null;

  const match = text.match(/^(on|off)\s+(.+)\s+hits=(\d+)\s*$/i);
  if (!match)
  {
    return {
      raw: text,
      state: 'unknown',
      enabled: false,
      module: '',
      method: '',
      line: '',
      hits: '',
      pattern: text
    };
  }

  const body = match[2];
  const lineMatch = body.match(/^(.*):(\d+)$/);
  const keyWithoutLine = lineMatch ? lineMatch[1] : body;
  const lineNumber = lineMatch ? lineMatch[2] : '';
  const moduleMatch = keyWithoutLine.match(/^(.*\.(?:c|cc|cpp|cxx|h|hpp|hxx)):(.*)$/i);
  const module = moduleMatch ? moduleMatch[1] : '';
  const method = moduleMatch ? moduleMatch[2] : keyWithoutLine;

  return {
    raw: text,
    state: match[1].toLowerCase(),
    enabled: match[1].toLowerCase() === 'on',
    module: module,
    method: method,
    line: lineNumber,
    hits: match[3],
    pattern: body
  };
}

function ParseTracePoints(text)
{
  return String(text || '')
    .split(/\r?\n/)
    .map(ParseTracePointLine)
    .filter(point => point !== null);
}

function TracePointRowHtml(point, index)
{
  const stateClass = point.enabled ? 'ok' : point.state === 'off' ? 'bad' : '';
  const stateText = point.state === 'unknown' ? 'UNKNOWN' : point.state.toUpperCase();
  const action = point.enabled ? 'disable' : 'enable';

  return `
    <tr data-trace-index="${index}">
      <td><span class="badge ${stateClass}">${EscapeHtml(stateText)}</span></td>
      <td class="trace-file-cell">${EscapeHtml(point.module || point.raw)}</td>
      <td class="trace-method-cell">${EscapeHtml(point.method || '')}</td>
      <td class="trace-line-cell">${EscapeHtml(point.line || '')}</td>
      <td class="trace-hits-cell">${EscapeHtml(point.hits || 'n/a')}</td>
      <td class="trace-actions-cell">
        <button class="secondary small-button" data-trace-action="${action}" data-trace-index="${index}">${point.enabled ? 'Disable' : 'Enable'}</button>
        <button class="secondary small-button" data-trace-action="reset" data-trace-index="${index}">Reset hits</button>
      </td>
    </tr>
  `;
}

function FilterTracePoints(points, filter)
{
  const needle = String(filter || '').toLowerCase();
  return points
    .map((point, index) => ({ point, index }))
    .filter(item => !needle || item.point.raw.toLowerCase().includes(needle));
}

function SortTracePointItems(items, sortKey, sortDir)
{
  const direction = sortDir === 'desc' ? -1 : 1;

  return items.slice().sort((a, b) =>
  {
    const left = a.point;
    const right = b.point;
    let result = 0;

    if (sortKey === 'state')
      result = left.state.localeCompare(right.state);
    else if (sortKey === 'file')
      result = left.module.localeCompare(right.module);
    else if (sortKey === 'method')
      result = left.method.localeCompare(right.method);
    else if (sortKey === 'line')
      result = (Number(left.line) || 0) - (Number(right.line) || 0);
    else if (sortKey === 'hits')
      result = (Number(left.hits) || 0) - (Number(right.hits) || 0);

    if (!result)
      result = left.raw.localeCompare(right.raw);

    return result * direction;
  });
}

function TraceSortMark(sortKey, currentKey, currentDir)
{
  if (sortKey !== currentKey)
    return '';

  return currentDir === 'desc' ? ' ↓' : ' ↑';
}

function RenderTracePointTable(points, filter, sortKey, sortDir)
{
  const filtered = FilterTracePoints(points, filter);
  const sorted = SortTracePointItems(filtered, sortKey, sortDir);

  if (!sorted.length)
    return '<div class="hint">No trace points match the filter.</div>';

  return `
    <div class="trace-table-wrap">
      <table class="trace-table">
        <thead>
          <tr>
            <th><button data-trace-sort="state">State${TraceSortMark('state', sortKey, sortDir)}</button></th>
            <th><button data-trace-sort="file">File${TraceSortMark('file', sortKey, sortDir)}</button></th>
            <th><button data-trace-sort="method">Function${TraceSortMark('method', sortKey, sortDir)}</button></th>
            <th><button data-trace-sort="line">Line${TraceSortMark('line', sortKey, sortDir)}</button></th>
            <th><button data-trace-sort="hits">Hits${TraceSortMark('hits', sortKey, sortDir)}</button></th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody>
          ${sorted.map(item => TracePointRowHtml(item.point, item.index)).join('')}
        </tbody>
      </table>
    </div>
  `;
}

async function ExecuteTraceCommand(command)
{
  const result = await SendCommand(command, 'text');
  if (!result.ok)
    throw new Error(result.error || result.text || 'trace command failed');

  return result;
}

async function RenderTracepoints()
{
  $('workArea').innerHTML = '<div class="loading">Loading trace points...</div>';

  const result = await SendCommand('trace list', 'text');
  const points = ParseTracePoints(result.text || '');

  $('workArea').innerHTML = `
    <div class="toolbar trace-toolbar">
      <input id="traceFilter" placeholder="Filter trace points by file, function or line...">
      <button id="traceReloadButton" class="secondary">Reload</button>
      <button id="traceEnableFilteredButton" class="secondary">Enable filtered</button>
      <button id="traceDisableFilteredButton" class="secondary">Disable filtered</button>
      <button id="traceResetFilteredButton" class="secondary">Reset filtered hits</button>
    </div>
    <div id="traceStatus"></div>
    <div id="traceOutput" class="result-card">
      <div class="result-header">
        <div>
          <div class="result-title">Trace points</div>
          <div class="command-line">trace list</div>
        </div>
        <span class="badge ${result.ok ? 'ok' : 'bad'}">${result.ok ? 'OK' : 'ERROR'}</span>
      </div>
      ${result.ok ? `
        <div id="traceSummary" class="trace-summary">${points.length} trace point(s)</div>
        <div id="tracePointList" class="trace-point-list"></div>
      ` : `
        <div class="error-text">${EscapeHtml(result.error || result.text || 'Trace command failed')}</div>
        <pre>${EscapeHtml(result.text || '')}</pre>
      `}
    </div>
  `;

  let sortKey = 'hits';
  let sortDir = 'desc';

  const refreshFilteredList = () =>
  {
    const list = $('tracePointList');
    if (!list)
      return;

    const filteredCount = FilterTracePoints(points, $('traceFilter').value).length;
    $('traceSummary').textContent = `${filteredCount} of ${points.length} trace point(s)`;
    list.innerHTML = RenderTracePointTable(points, $('traceFilter').value, sortKey, sortDir);
    BindTracePointActionButtons(points);
    BindTraceSortButtons(points, () => sortKey, value => { sortKey = value; }, () => sortDir, value => { sortDir = value; }, refreshFilteredList);
  };

  $('traceFilter').addEventListener('input', refreshFilteredList);
  $('traceReloadButton').addEventListener('click', () => RenderTracepoints());

  $('traceEnableFilteredButton').addEventListener('click', async () =>
  {
    await ExecuteTraceFilteredCommand(points, 'enable');
  });

  $('traceDisableFilteredButton').addEventListener('click', async () =>
  {
    await ExecuteTraceFilteredCommand(points, 'disable');
  });

  $('traceResetFilteredButton').addEventListener('click', async () =>
  {
    await ExecuteTraceFilteredCommand(points, 'reset');
  });

  refreshFilteredList();
}

async function ExecuteTraceToolbarCommand(command)
{
  const status = $('traceStatus');
  status.innerHTML = '<div class="loading">Sending command...</div>';

  try
  {
    const result = await ExecuteTraceCommand(command);
    status.innerHTML = `<div class="success-card">${EscapeHtml(result.text || 'ok')}</div>`;
    await RenderTracepoints();
  }
  catch (e)
  {
    status.innerHTML = `<div class="error-card">${EscapeHtml(e.message || e)}</div>`;
  }
}


async function ExecuteTraceFilteredCommand(points, action)
{
  const items = FilterTracePoints(points, $('traceFilter').value);
  const status = $('traceStatus');

  if (!items.length)
  {
    status.innerHTML = '<div class="error-card">No trace points match the filter.</div>';
    return;
  }

  status.innerHTML = `<div class="loading">Sending ${items.length} command(s)...</div>`;

  try
  {
    for (const item of items)
      await ExecuteTraceCommand(`trace ${action} ${TracePointPatternArg(item.point.pattern)}`);

    status.innerHTML = `<div class="success-card">${EscapeHtml(action)} completed for ${items.length} trace point(s)</div>`;
    await RenderTracepoints();
  }
  catch (e)
  {
    status.innerHTML = `<div class="error-card">${EscapeHtml(e.message || e)}</div>`;
  }
}

function BindTraceSortButtons(points, getSortKey, setSortKey, getSortDir, setSortDir, refresh)
{
  document.querySelectorAll('[data-trace-sort]').forEach(button =>
  {
    button.addEventListener('click', () =>
    {
      const key = button.dataset.traceSort;
      if (getSortKey() === key)
        setSortDir(getSortDir() === 'asc' ? 'desc' : 'asc');
      else
      {
        setSortKey(key);
        setSortDir(key === 'hits' ? 'desc' : 'asc');
      }

      refresh();
    });
  });
}

function BindTracePointActionButtons(points)
{
  document.querySelectorAll('[data-trace-action]').forEach(button =>
  {
    button.addEventListener('click', async () =>
    {
      const index = Number(button.dataset.traceIndex);
      const point = points[index];
      if (!point)
        return;

      const action = button.dataset.traceAction;
      const command = `trace ${action} ${TracePointPatternArg(point.pattern)}`;
      await ExecuteTraceToolbarCommand(command);
    });
  });
}

async function RenderBackends()
{
  $('workArea').innerHTML = '<div class="loading">Loading backends...</div>';

  const channels = await LoadChannels();
  let html = `
    <div class="split-view">
      <div class="list-panel">
        <div class="section-title">Backends by channel</div>
        <div class="hint">Backend commands are channel-specific. Select a channel to inspect or change its backends.</div>
        <div class="item-list" id="backendChannelList">
  `;

  for (const channel of channels)
  {
    html += `
      <button class="list-item" data-channel="${EscapeHtml(channel)}">
        <span>${EscapeHtml(ChannelNameForDisplay(channel))}</span>
      </button>
    `;
  }

  html += `
        </div>
      </div>
      <div class="detail-panel" id="backendDetails">
        <div class="empty-state">Select a channel to see its backends.</div>
      </div>
    </div>
  `;

  $('workArea').innerHTML = html;

  document.querySelectorAll('#backendChannelList .list-item').forEach(button =>
  {
    button.addEventListener('click', async () =>
    {
      document.querySelectorAll('#backendChannelList .list-item').forEach(x => x.classList.remove('selected'));
      button.classList.add('selected');

      const channel = button.dataset.channel;
      const command = channel ? `channel ${QuoteArg(channel)}` : 'channel';
      $('backendDetails').innerHTML = '<div class="loading">Loading channel information...</div>';

      try
      {
        const result = await SendCommand(command, 'text');
        const backendTypes = ParseBackendTypes(result.text || '');
        const backendHtml = Object.keys(backendTypes).length
          ? Object.entries(backendTypes)
            .sort((a, b) => a[0].localeCompare(b[0]))
            .map(([name, count]) => `<div class="metric-line"><span>${EscapeHtml(name)}</span><strong>${EscapeHtml(count)}</strong></div>`)
            .join('')
          : '<div class="empty-state small">No backends found in channel details</div>';

        $('backendDetails').innerHTML = `
          <div class="card soft">
            <h2>${EscapeHtml(ChannelNameForDisplay(channel))}</h2>
            <div class="hint">Backend command requires an operation, so this page reads backend information from channel details.</div>
            <div class="metric-list">${backendHtml}</div>
          </div>
          ${RenderResponseCard('Channel details', command, result)}
          <div class="hint">Use the Channels page for backend add/delete operations on this channel.</div>
        `;
      }
      catch (e)
      {
        $('backendDetails').innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
      }
    });
  });
}

function BuildChannelSelect(channels)
{
  return `
    <label>Channel</label>
    <select id="builderChannel">
      ${channels.map(channel => `<option value="${EscapeHtml(channel)}">${EscapeHtml(ChannelNameForDisplay(channel))}</option>`).join('')}
    </select>
    <div class="hint compact">For the default channel the command is sent without --channel.</div>
  `;
}

function SetManualCommand(text)
{
  $('manualCommandText').value = text;
}

function GetBuilderChannelArg()
{
  const channel = $('builderChannel') ? $('builderChannel').value : '';
  return ChannelOptionArg(channel);
}

function UpdateManualBuilder()
{
  const preset = $('commandPreset').value;
  const box = $('commandBuilder');
  const channels = CachedChannels || [''];

  if (preset === 'help')
  {
    box.innerHTML = '<div class="hint">Help is shown here because it is mainly useful while composing manual commands.</div>';
    SetManualCommand('help');
    return;
  }

  if (preset === 'overview')
  {
    box.innerHTML = '<div class="hint">Displays runtime logging summary.</div>';
    SetManualCommand('overview');
    return;
  }

  if (preset === 'list')
  {
    box.innerHTML = '<div class="hint">Lists all channels.</div>';
    SetManualCommand('list');
    return;
  }

  if (preset === 'channel-info')
  {
    box.innerHTML = BuildChannelSelect(channels);
    const update = () =>
    {
      const channel = $('builderChannel').value;
      SetManualCommand(channel ? `channel ${QuoteArg(channel)}` : 'channel');
    };
    $('builderChannel').addEventListener('change', update);
    update();
    return;
  }

  if (preset === 'channel-op')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderChannelOp">
        <option value="--create">create</option>
        <option value="--delete">delete</option>
        <option value="--enable">enable</option>
        <option value="--disable">disable</option>
        <option value="--bind">bind</option>
        <option value="--unbind">unbind</option>
        <option value="--error">set error channel</option>
        <option value="--clear-error">clear error channel</option>
      </select>
      <div id="builderChannelTarget"></div>
    `;

    const update = () =>
    {
      const op = $('builderChannelOp').value;

      if (op === '--clear-error')
      {
        SetManualCommand('channel --clear-error');
        return;
      }

      if (op === '--bind')
      {
        const source = $('builderSourceChannel') ? $('builderSourceChannel').value : '';
        const target = $('builderTargetChannel') ? $('builderTargetChannel').value : '';
        SetManualCommand(`channel --bind ${ChannelControlNameArg(source)} ${ChannelControlNameArg(target)}`);
        return;
      }

      const name = op === '--create'
        ? ($('builderChannelName') ? $('builderChannelName').value : '')
        : ($('builderChannel') ? $('builderChannel').value : '');
      const channelName = op === '--create'
        ? QuoteArg(name)
        : ChannelControlNameArg(name);

      SetManualCommand(`channel ${op} ${channelName}`.trim());
    };

    const renderTarget = () =>
    {
      const op = $('builderChannelOp').value;
      const target = $('builderChannelTarget');

      if (op === '--clear-error')
      {
        target.innerHTML = '<div class="hint compact">This command does not take a channel name.</div>';
      }
      else if (op === '--create')
      {
        target.innerHTML = `
          <label>New channel name</label>
          <input id="builderChannelName" placeholder="channel name">
        `;
        $('builderChannelName').addEventListener('input', update);
      }
      else if (op === '--bind')
      {
        target.innerHTML = `
          <label>Source channel</label>
          <select id="builderSourceChannel">
            ${channels.map(channel => `<option value="${EscapeHtml(channel)}">${EscapeHtml(ChannelNameForDisplay(channel))}</option>`).join('')}
          </select>
          <label>Target channel</label>
          <select id="builderTargetChannel">
            ${channels.map(channel => `<option value="${EscapeHtml(channel)}">${EscapeHtml(ChannelNameForDisplay(channel))}</option>`).join('')}
          </select>
        `;
        $('builderSourceChannel').addEventListener('change', update);
        $('builderTargetChannel').addEventListener('change', update);
      }
      else
      {
        target.innerHTML = BuildChannelSelect(channels);
        $('builderChannel').addEventListener('change', update);
      }

      update();
    };

    $('builderChannelOp').addEventListener('change', renderTarget);
    renderTarget();
    return;
  }

  if (preset === 'flags')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Flags arguments</label>
      <input id="builderArgs" placeholder="empty = display flags, or flag[=value] ...">
    `;
    const update = () => SetManualCommand(`flags${GetBuilderChannelArg()} ${$('builderArgs').value}`.trim());
    $('builderChannel').addEventListener('change', update);
    $('builderArgs').addEventListener('input', update);
    update();
    return;
  }

  if (preset === 'level')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Level</label>
      <select id="builderLevel">
        <option value="">display current level</option>
        <option value="debug">debug</option>
        <option value="info">info</option>
        <option value="warn">warn</option>
        <option value="error">error</option>
        <option value="critical">critical</option>
      </select>
    `;
    const update = () => SetManualCommand(`level${GetBuilderChannelArg()} ${$('builderLevel').value}`.trim());
    $('builderChannel').addEventListener('change', update);
    $('builderLevel').addEventListener('change', update);
    update();
    return;
  }

  if (preset === 'backend')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Operation</label>
      <select id="builderBackendOp">
        <option value="--add">add</option>
        <option value="--delete">delete</option>
      </select>
      <label>Backend type</label>
      <select id="builderBackendType">
        <option value="ConsoleBackend">ConsoleBackend</option>
        <option value="DebugBackend">DebugBackend</option>
        <option value="FileBackend">FileBackend</option>
        <option value="SharedFileBackend">SharedFileBackend</option>
        <option value="BufferBackend">BufferBackend</option>
      </select>
      <div id="builderBackendOptions"></div>
    `;

    const update = () =>
    {
      let command = `backend${GetBuilderChannelArg()} ${$('builderBackendOp').value} ${$('builderBackendType').value}`;

      if ($('builderBackendOp').value === '--add')
      {
        const type = $('builderBackendType').value;

        if (type === 'ConsoleBackend' && $('builderBackendAsync').checked)
          command += ' --async';

        if ((type === 'FileBackend' || type === 'SharedFileBackend') && $('builderBackendFile').value)
          command += ` --file ${QuoteArg($('builderBackendFile').value)}`;

        if ((type === 'FileBackend' || type === 'SharedFileBackend' || type === 'BufferBackend') && $('builderBackendMaxSize') && $('builderBackendMaxSize').value)
          command += ` --max-size ${QuoteArg($('builderBackendMaxSize').value)}`;

        if (type === 'FileBackend')
        {
          command += ` ${$('builderBackendFileMode').value}`;
          command += $('builderBackendDailyRotation').checked ? ' --daily-rotation' : ' --no-daily-rotation';

          if ($('builderBackendMaxParts').value)
            command += ` --max-parts ${QuoteArg($('builderBackendMaxParts').value)}`;
        }

        if (type === 'SharedFileBackend' && $('builderBackendTimeout').value)
          command += ` --timeout ${QuoteArg($('builderBackendTimeout').value)}`;

        if (type === 'BufferBackend')
          command += ` --policy ${$('builderBackendPolicy').value}`;
      }

      SetManualCommand(command);
    };

    const renderOptions = () =>
    {
      const op = $('builderBackendOp').value;
      const type = $('builderBackendType').value;
      const options = $('builderBackendOptions');

      if (op !== '--add')
      {
        options.innerHTML = '<div class="hint compact">Delete does not use backend creation options.</div>';
      }
      else if (type === 'ConsoleBackend')
      {
        options.innerHTML = `
          <label class="checkbox-line"><input id="builderBackendAsync" type="checkbox"> async</label>
        `;
        $('builderBackendAsync').addEventListener('change', update);
      }
      else if (type === 'FileBackend')
      {
        options.innerHTML = `
          <label>File name</label>
          <input id="builderBackendFile" placeholder="optional output file path">
          <label>File mode</label>
          <select id="builderBackendFileMode">
            <option value="--append">append</option>
            <option value="--overwrite">overwrite</option>
          </select>
          <label>Max size</label>
          <input id="builderBackendMaxSize" placeholder="8mb">
          <label class="checkbox-line"><input id="builderBackendDailyRotation" type="checkbox"> daily rotation</label>
          <label>Max parts</label>
          <input id="builderBackendMaxParts" placeholder="2">
        `;
        $('builderBackendFile').addEventListener('input', update);
        $('builderBackendFileMode').addEventListener('change', update);
        $('builderBackendMaxSize').addEventListener('input', update);
        $('builderBackendDailyRotation').addEventListener('change', update);
        $('builderBackendMaxParts').addEventListener('input', update);
      }
      else if (type === 'SharedFileBackend')
      {
        options.innerHTML = `
          <label>File name</label>
          <input id="builderBackendFile" placeholder="optional output file path">
          <label>Max size</label>
          <input id="builderBackendMaxSize" placeholder="8mb">
          <label>Timeout</label>
          <input id="builderBackendTimeout" placeholder="10ms">
        `;
        $('builderBackendFile').addEventListener('input', update);
        $('builderBackendMaxSize').addEventListener('input', update);
        $('builderBackendTimeout').addEventListener('input', update);
      }
      else if (type === 'BufferBackend')
      {
        options.innerHTML = `
          <label>Max size</label>
          <input id="builderBackendMaxSize" placeholder="4mb">
          <label>Policy</label>
          <select id="builderBackendPolicy">
            <option value="stop-appending">stop-appending</option>
            <option value="delete-oldest">delete-oldest</option>
          </select>
        `;
        $('builderBackendMaxSize').addEventListener('input', update);
        $('builderBackendPolicy').addEventListener('change', update);
      }
      else
      {
        options.innerHTML = '<div class="hint compact">This backend type has no Manual command creation options.</div>';
      }

      update();
    };

    $('builderChannel').addEventListener('change', update);
    $('builderBackendOp').addEventListener('change', renderOptions);
    $('builderBackendType').addEventListener('change', renderOptions);
    renderOptions();
    return;
  }

  if (preset === 'subsystem')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderSubsystemOp">
        <option value="">display current filters</option>
        <option value="--block">block</option>
        <option value="--unblock">unblock</option>
        <option value="--allow">allow</option>
        <option value="--disallow">disallow</option>
        <option value="--check">check</option>
        <option value="--clear">clear all</option>
        <option value="--clear-blocked">clear blocked</option>
        <option value="--clear-allowed">clear allowed</option>
      </select>
      <label>Subsystem name</label>
      <input id="builderSubsystemName" placeholder="used by block/unblock/allow/disallow/check">
    `;
    const update = () =>
    {
      const op = $('builderSubsystemOp').value;
      const name = $('builderSubsystemName').value;
      const needsName = ['--block', '--unblock', '--allow', '--disallow', '--check'].includes(op);
      SetManualCommand(`subsystem ${op}${needsName ? ' ' + QuoteArg(name) : ''}`.trim());
    };
    $('builderSubsystemOp').addEventListener('change', update);
    $('builderSubsystemName').addEventListener('input', update);
    update();
    return;
  }

  if (preset === 'trace')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderTraceOp">
        <option value="">list</option>
        <option value="list">list</option>
        <option value="stat">stat</option>
        <option value="stats">stats</option>
        <option value="enable">enable</option>
        <option value="disable">disable</option>
        <option value="reset">reset</option>
      </select>
      <label>Pattern</label>
      <input id="builderTracePattern" placeholder="optional wildcard pattern">
    `;
    const update = () => SetManualCommand(`trace ${$('builderTraceOp').value} ${$('builderTracePattern').value}`.trim());
    $('builderTraceOp').addEventListener('change', update);
    $('builderTracePattern').addEventListener('input', update);
    update();
    return;
  }

  box.innerHTML = '<div class="hint">Custom mode leaves the command text fully editable.</div>';
  SetManualCommand('');
}

async function RenderManual()
{
  $('workArea').innerHTML = '<div class="loading">Loading command builder...</div>';

  try
  {
    await LoadChannels();
  }
  catch (e)
  {
    CachedChannels = [''];
  }

  $('workArea').innerHTML = `
    <div class="manual-layout">
      <div class="manual-command card soft">
        <h2>Manual command</h2>
        <p>Select a command and fill the fields. The final command text remains editable before sending.</p>

        <label>Command preset</label>
        <select id="commandPreset">
          <option value="help">help</option>
          <option value="overview">overview</option>
          <option value="list">list</option>
          <option value="channel-info">channel information</option>
          <option value="channel-op">channel operation</option>
          <option value="flags">flags</option>
          <option value="level">level</option>
          <option value="backend">backend</option>
          <option value="subsystem">subsystem</option>
          <option value="trace">trace</option>
          <option value="custom">custom</option>
        </select>

        <div id="commandBuilder" class="command-builder"></div>

        <label>Command text</label>
        <input id="manualCommandText" value="help">

        <label>Format</label>
        <select id="manualFormat">
          <option value="text">Text</option>
          <option value="json">JSON</option>
        </select>

        <button class="primary manual-send-button" id="sendManualButton">Send command</button>
      </div>

      <div id="manualResult" class="manual-result"></div>
    </div>
  `;

  async function SendManualCommand()
  {
    const command = $('manualCommandText').value;
    const format = $('manualFormat').value;
    $('manualResult').innerHTML = '<div class="loading">Sending command...</div>';

    try
    {
      const result = await SendCommand(command, format);
      $('manualResult').innerHTML = RenderResponseCard('Manual command result', command, result);
    }
    catch (e)
    {
      $('manualResult').innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
    }
  }

  $('commandPreset').addEventListener('change', UpdateManualBuilder);
  UpdateManualBuilder();

  $('sendManualButton').addEventListener('click', SendManualCommand);
  await SendManualCommand();
}


const LOG_INITIAL_CHUNK_SIZE = 4 * 1024;
const LOG_BACKGROUND_CHUNK_SIZE = 64 * 1024;
const LOG_PAGE_CHUNK_SIZE = 512 * 1024;
let CurrentLogState = {
  treePath: '',
  filePath: '',
  offset: 0,
  limit: LOG_PAGE_CHUNK_SIZE,
  size: 0,
  loadedEnd: 0,
  content: '',
  nowrap: true,
  layout: 'vertical',
  search: '',
  loading: false,
  loadGeneration: 0,
  renderLevel: 'info',
  loadError: '',
  searchMatches: [],
  searchIndex: -1
};

function ParseLogTree(text)
{
  const result = {
    home: '',
    path: '',
    dirs: [],
    files: []
  };

  for (const line of String(text || '').split(/\r?\n/))
  {
    if (line.startsWith('Home directory:'))
    {
      result.home = line.substring('Home directory:'.length).trim();
      continue;
    }

    if (line.startsWith('Path:'))
    {
      result.path = line.substring('Path:'.length).trim();
      continue;
    }

    if (line.startsWith('DIR\t'))
    {
      result.dirs.push(line.substring(4));
      continue;
    }

    if (line.startsWith('FILE\t'))
    {
      const parts = line.split('\t');
      result.files.push({
        path: parts[1] || '',
        size: Number(parts[2] || 0),
        time: parts[3] || ''
      });
    }
  }

  return result;
}

function ParseLogRange(text)
{
  const value = String(text || '');
  const lineEnd = value.indexOf('\n');
  if (!value.startsWith('LOGMEWEB-RANGE\t') || lineEnd < 0)
  {
    return {
      offset: 0,
      limit: LOG_PAGE_CHUNK_SIZE,
      size: value.length,
      content: value
    };
  }

  const header = value.substring(0, lineEnd).split('\t');
  return {
    offset: Number(header[1] || 0),
    limit: Number(header[2] || LOG_PAGE_CHUNK_SIZE),
    size: Number(header[3] || 0),
    content: value.substring(lineEnd + 1)
  };
}

function LogPathArg(path)
{
  return String(path || '').replace(/[\r\n\t]/g, ' ');
}

function GetParentLogPath(path)
{
  const text = String(path || '').replaceAll('\\', '/');
  const pos = text.lastIndexOf('/');
  if (pos < 0)
    return '';

  return text.substring(0, pos);
}

function GetLogBaseName(path)
{
  const text = String(path || '').replaceAll('\\', '/');
  const pos = text.lastIndexOf('/');
  if (pos < 0)
    return text;

  return text.substring(pos + 1);
}

function GetSelectedLogFile(tree, selectedPath)
{
  if (!tree || !selectedPath)
    return null;

  return (tree.files || []).find(file => file.path === selectedPath) || null;
}

function FormatLogTime(value)
{
  const text = String(value || '').trim();
  if (!text)
    return '';

  const numeric = Number(text);
  if (Number.isFinite(numeric) && numeric > 0)
  {
    let milliseconds = numeric;

    // Windows FILETIME: 100 ns ticks since 1601-01-01.
    if (numeric > 100000000000000000)
      milliseconds = (numeric / 10000) - 11644473600000;
    else if (numeric > 1000000000000)
      milliseconds = numeric;
    else
      milliseconds = numeric * 1000;

    const date = new Date(milliseconds);
    if (!Number.isNaN(date.getTime()))
      return FormatDateTime(date);
  }

  const date = new Date(text);
  if (!Number.isNaN(date.getTime()))
    return FormatDateTime(date);

  return text;
}

function Pad2(value)
{
  return String(value).padStart(2, '0');
}

function FormatDateTime(date)
{
  return [
    date.getFullYear()
    , '-'
    , Pad2(date.getMonth() + 1)
    , '-'
    , Pad2(date.getDate())
    , ' '
    , Pad2(date.getHours())
    , ':'
    , Pad2(date.getMinutes())
    , ':'
    , Pad2(date.getSeconds())
  ].join('');
}


function HighlightLogSearch(text, query)
{
  const escaped = EscapeHtml(text || ' ');
  if (!query)
    return escaped;

  const needle = EscapeHtml(query);
  const pattern = new RegExp(needle.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi');
  return escaped.replace(pattern, match => `<mark>${match}</mark>`);
}

function GetLogmeLineInfo(line)
{
  const text = String(line || '');

  const match = text.match(
    /^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}[:.]\d{3}\s+)([DIWEC])?(\s+.*?\]\s*)(.*)$/
  );

  if (!match)
    return null;

  const level = (match[2] || 'I').toUpperCase();
  let name = 'info';

  if (level === 'D')
    name = 'debug';
  else if (level === 'W')
    name = 'warn';
  else if (level === 'E')
    name = 'error';
  else if (level === 'C')
    name = 'critical';

  return {
    level: name,
    prefix: match[1] + (match[2] || ' ') + match[3],
    message: match[4] || ''
  };
}

function DetectLogmeLineLevel(line)
{
  const text = String(line || '');
  const logmeInfo = GetLogmeLineInfo(text);
  if (logmeInfo)
    return logmeInfo.level;

  if (/\b(CRITICAL|FATAL)\b/i.test(text))
    return 'critical';

  if (/\b(ERROR|ERR)\b/i.test(text))
    return 'error';

  if (/\b(WARN|WARNING)\b/i.test(text))
    return 'warn';

  if (/\b(DEBUG|TRACE)\b/i.test(text))
    return 'debug';

  if (/\bINFO\b/i.test(text))
    return 'info';

  return '';
}

function LogLevelClass(level)
{
  if (level === 'critical')
    return 'log-critical';

  if (level === 'error')
    return 'log-error';

  if (level === 'warn')
    return 'log-warn';

  if (level === 'debug')
    return 'log-debug';

  return 'log-info';
}

function RenderLogLine(line, query = '', inheritedLevel = 'info')
{
  const text = String(line || '');
  const logmeInfo = GetLogmeLineInfo(text);
  const level = logmeInfo ? logmeInfo.level : (DetectLogmeLineLevel(text) || inheritedLevel || 'info');

  if (logmeInfo)
  {
    return `<div class="log-line ${LogLevelClass(level)}"><span class="log-system">${HighlightLogSearch(logmeInfo.prefix, query)}</span>${HighlightLogSearch(logmeInfo.message, query)}</div>`;
  }

  return `<div class="log-line ${LogLevelClass(level)}">${HighlightLogSearch(text, query)}</div>`;
}

function RenderLogTextPart(text, query = '', initialLevel = 'info')
{
  const lines = String(text || '').split(/\r?\n/);
  let currentLevel = initialLevel || 'info';

  const html = lines.map(line =>
  {
    const level = DetectLogmeLineLevel(line);
    if (level)
      currentLevel = level;

    return RenderLogLine(line, query, currentLevel);
  }).join('');

  return { html, level: currentLevel };
}

function RenderLogText(text, query = '')
{
  return RenderLogTextPart(text, query, 'info').html;
}

async function LoadLogsTree(path = '')
{
  const command = path ? `logs --tree ${LogPathArg(path)}` : 'logs --tree';
  const result = await SendCommand(command, 'text');
  if (!result.ok)
    throw new Error(result.error || result.text || 'unable to load log tree');

  return ParseLogTree(result.text || '');
}

async function LoadLogRange(path, offset = 0, limit = LOG_CHUNK_SIZE)
{
  const command = `logs --read ${LogPathArg(path)} ${Math.max(0, offset)} ${limit}`;
  const result = await SendCommand(command, 'text');
  if (!result.ok)
    throw new Error(result.error || result.text || 'unable to load log file');

  return ParseLogRange(result.text || '');
}

function DecodeBase64ToBytes(text)
{
  const binary = atob(text || '');
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; ++i)
    bytes[i] = binary.charCodeAt(i);

  return bytes;
}

async function DownloadLogFile(path)
{
  const result = await SendCommand(`logs --download ${LogPathArg(path)}`, 'text');
  if (!result.ok)
    throw new Error(result.error || result.text || 'unable to download log file');

  const text = result.text || '';
  const lineEnd = text.indexOf('\n');
  if (!text.startsWith('LOGMEWEB-DOWNLOAD-B64\t') || lineEnd < 0)
    throw new Error('invalid download response');

  const bytes = DecodeBase64ToBytes(text.substring(lineEnd + 1).trim());
  const blob = new Blob([bytes], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = GetLogBaseName(path) || 'log.txt';
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

function GetLoadedEndOffset()
{
  return Math.min(Number(CurrentLogState.size || 0), Number(CurrentLogState.loadedEnd || 0));
}

function IsLogFullyLoaded()
{
  const size = Number(CurrentLogState.size || 0);
  if (!CurrentLogState.filePath)
    return false;

  return size > 0 && GetLoadedEndOffset() >= size;
}

function UpdateLogLoadingStatus()
{
  const status = $('logLoadingStatus');
  const position = $('logPositionStatus');

  if (status)
  {
    status.textContent = '';
    status.classList.add('hidden');
  }

  if (position)
  {
    position.textContent = '';
    position.classList.add('hidden');
  }

  const size = Number(CurrentLogState.size || 0);
  const loadedEnd = GetLoadedEndOffset();
  const offset = Number(CurrentLogState.offset || 0);
  const loadedBytes = Math.max(0, loadedEnd - offset);
  const percent = size ? Math.floor((loadedEnd / size) * 100) : 0;
  const isCompleteFile = CurrentLogState.filePath && size > 0 && offset === 0 && loadedEnd >= size;

  if (position && CurrentLogState.filePath && !isCompleteFile)
  {
    position.textContent = `${FormatBytes(offset)} - ${FormatBytes(loadedEnd)} of ${FormatBytes(size)} (${percent}%)`;
    position.classList.remove('hidden');
  }

  if (!status)
    return;

  if (CurrentLogState.loadError)
  {
    status.textContent = `Loading stopped: ${CurrentLogState.loadError}`;
    status.classList.remove('hidden');
    return;
  }

  if (CurrentLogState.loading)
  {
    status.textContent = `Loading... ${FormatBytes(loadedBytes)} loaded, ${percent}% of file`;
    status.classList.remove('hidden');
  }
}

function AppendLogContent(text)
{
  const chunk = String(text || '');
  if (!chunk)
    return;

  CurrentLogState.content += chunk;

  const viewer = document.querySelector('.log-viewer');
  if (!viewer)
    return;

  if (CurrentLogState.search)
  {
    RefreshLogSearch(false);
    return;
  }

  const rendered = RenderLogTextPart(chunk, '', CurrentLogState.renderLevel);
  viewer.insertAdjacentHTML('beforeend', rendered.html);
  CurrentLogState.renderLevel = rendered.level;
}


function UpdateLogSearchNavigation()
{
  const status = $('logSearchStatus');
  if (!status)
    return;

  const query = CurrentLogState.search || '';
  if (!query)
  {
    status.textContent = '';
    return;
  }

  const text = CurrentLogState.content || '';
  const lowerText = text.toLowerCase();
  const lowerQuery = query.toLowerCase();
  const matches = [];
  let pos = lowerText.indexOf(lowerQuery);

  while (pos >= 0 && matches.length < 10000)
  {
    matches.push(pos);
    pos = lowerText.indexOf(lowerQuery, pos + Math.max(1, lowerQuery.length));
  }

  CurrentLogState.searchMatches = matches;

  if (!matches.length)
  {
    CurrentLogState.searchIndex = -1;
    status.textContent = '0 matches';
    return;
  }

  if (CurrentLogState.searchIndex < 0 || CurrentLogState.searchIndex >= matches.length)
    CurrentLogState.searchIndex = 0;

  status.textContent = `${CurrentLogState.searchIndex + 1} of ${matches.length}`;
}

function ScrollToSearchMatch(index)
{
  const matches = CurrentLogState.searchMatches || [];
  if (index < 0 || index >= matches.length)
    return;

  const textBefore = (CurrentLogState.content || '').substring(0, matches[index]);
  const lineIndex = textBefore.split(/\r?\n/).length - 1;
  const viewer = document.querySelector('.log-viewer');
  const line = viewer ? viewer.querySelectorAll('.log-line')[lineIndex] : null;

  if (line)
    line.scrollIntoView({ block: 'center' });
}

function RefreshLogSearch(scrollToFirst = false)
{
  const viewer = document.querySelector('.log-viewer');
  if (viewer)
    viewer.innerHTML = RenderLogText(CurrentLogState.content, CurrentLogState.search);

  UpdateLogSearchNavigation();

  if (scrollToFirst && (CurrentLogState.searchMatches || []).length)
    ScrollToSearchMatch(CurrentLogState.searchIndex);
}


function ResetLogContent()
{
  CurrentLogState.content = '';
  CurrentLogState.loadedEnd = CurrentLogState.offset;
  CurrentLogState.renderLevel = 'info';
  CurrentLogState.loadError = '';
  CurrentLogState.searchMatches = [];
  CurrentLogState.searchIndex = -1;

  const viewer = document.querySelector('.log-viewer');
  if (viewer)
    viewer.innerHTML = '';
}

async function Delay(ms)
{
  await new Promise(resolve => setTimeout(resolve, ms));
}

async function StartProgressiveLogLoad(path, offset = 0)
{
  const generation = ++CurrentLogState.loadGeneration;
  CurrentLogState.loading = true;
  CurrentLogState.filePath = path;
  CurrentLogState.offset = Math.max(0, offset || 0);
  CurrentLogState.limit = LOG_PAGE_CHUNK_SIZE;
  ResetLogContent();
  UpdateLogLoadingStatus();

  try
  {
    let nextOffset = CurrentLogState.offset;
    let limit = LOG_INITIAL_CHUNK_SIZE;

    while (generation === CurrentLogState.loadGeneration)
    {
      const range = await LoadLogRange(path, nextOffset, limit);
      if (generation !== CurrentLogState.loadGeneration)
        return;

      CurrentLogState.size = range.size;
      CurrentLogState.loadedEnd = Math.min(range.size, range.offset + range.limit);
      AppendLogContent(range.content);
      UpdateLogLoadingStatus();

      if (CurrentLogState.loadedEnd >= range.size)
        break;

      nextOffset = CurrentLogState.loadedEnd;
      limit = LOG_BACKGROUND_CHUNK_SIZE;
      await Delay(25);
    }
  }
  catch (e)
  {
    if (generation === CurrentLogState.loadGeneration)
      CurrentLogState.loadError = e.message || String(e);
  }
  finally
  {
    if (generation === CurrentLogState.loadGeneration)
    {
      CurrentLogState.loading = false;
      UpdateLogLoadingStatus();
      RenderLogsLayout(CurrentLogState.tree || { home: '', path: '', dirs: [], files: [] }, path);
    }
  }
}

function StopLogLoading()
{
  ++CurrentLogState.loadGeneration;
  CurrentLogState.loading = false;
  UpdateLogLoadingStatus();
}

function RenderLogViewer(tree, selectedPath = '')
{
  const state = CurrentLogState;
  const offset = Number(state.offset || 0);
  const size = Number(state.size || 0);
  const endOffset = GetLoadedEndOffset();
  const percent = size ? Math.floor((endOffset / size) * 100) : 0;
  const isCompleteFile = selectedPath && size > 0 && offset === 0 && endOffset >= size;
  const logPositionText = `${FormatBytes(offset)} - ${FormatBytes(endOffset)} of ${FormatBytes(size)} (${percent}%)`;
  const logPositionClass = isCompleteFile ? 'muted log-position hidden' : 'muted log-position';
  const selectedFile = GetSelectedLogFile(tree, selectedPath);
  const modifiedTime = selectedFile ? FormatLogTime(selectedFile.time) : '';
  const viewer = selectedPath
    ? RenderLogText(state.content, state.search)
    : '<div class="empty-state">Select a log file to view it from the beginning.</div>';
  const noWrapClass = state.nowrap ? 'nowrap' : '';
  const layoutClass = state.layout === 'horizontal' ? 'horizontal' : 'vertical';

  return `
    <div class="logs-layout ${layoutClass}">
      ${RenderLogBrowser(tree, selectedPath)}

      <div class="card soft logs-viewer-card">
        <div class="logs-viewer-header">
          <div>
            <h2>${EscapeHtml(selectedPath ? GetLogBaseName(selectedPath) : 'Log viewer')}</h2>
            <div class="muted">${EscapeHtml(selectedPath || '')}</div>
            ${modifiedTime ? `<div class="muted log-modified">Modified: ${EscapeHtml(modifiedTime)}</div>` : ''}
          </div>
          <div class="logs-viewer-actions">
            <button class="secondary small" id="toggleLogsLayoutButton">${state.layout === 'horizontal' ? 'Tree on top' : 'Side by side'}</button>
            ${selectedPath ? '<button class="secondary small" id="toggleLogWrapButton">' + (state.nowrap ? 'Wrap lines' : 'No wrap') + '</button>' : ''}
            ${selectedPath ? '<button class="secondary small" id="downloadLogFileButton">Download</button>' : ''}
            ${selectedPath ? '<button class="secondary small" id="reloadLogFileButton">Reload</button>' : ''}
            ${selectedPath && state.loading ? '<button class="secondary small" id="stopLogLoadingButton">Stop loading</button>' : ''}
          </div>
        </div>

        ${selectedPath ? `
          <div class="log-navigation">
            <button class="secondary small" id="firstLogChunkButton">First</button>
            <button class="secondary small" id="prevLogChunkButton">Previous</button>
            <button class="secondary small" id="nextLogChunkButton">Next</button>
            <button class="secondary small" id="lastLogChunkButton">Last</button>
            <input id="logSearchInput" placeholder="Search in loaded text" value="${EscapeHtml(state.search || '')}">
            <button class="secondary small" id="prevLogSearchButton">Previous match</button>
            <button class="secondary small" id="nextLogSearchButton">Next match</button>
            <span class="muted" id="logSearchStatus"></span>
            <span class="${logPositionClass}" id="logPositionStatus">${EscapeHtml(logPositionText)}</span>
            <span class="muted log-loading-status hidden" id="logLoadingStatus"></span>
          </div>
        ` : ''}

        <div class="log-viewer ${noWrapClass}">${viewer}</div>
      </div>
    </div>
  `;
}

function RenderLogBrowser(tree, selectedPath = '')
{
  const parent = GetParentLogPath(tree.path);
  const upButton = tree.path
    ? `<button class="secondary small log-folder-button" data-log-folder="${EscapeHtml(parent)}">..</button>`
    : '';

  const dirsHtml = tree.dirs.map(path => `
    <button class="log-tree-item log-folder-button" data-log-folder="${EscapeHtml(path)}">
      <span class="log-tree-icon">▸</span>
      <span>${EscapeHtml(GetLogBaseName(path) || path)}</span>
    </button>
  `).join('');

  const filesHtml = tree.files.map(file => `
    <button class="log-tree-item log-file-button ${file.path === selectedPath ? 'selected' : ''}" data-log-file="${EscapeHtml(file.path)}">
      <span class="log-tree-icon">•</span>
      <span class="log-file-name">${EscapeHtml(GetLogBaseName(file.path) || file.path)}</span>
      <span class="log-file-size">${EscapeHtml(FormatBytes(file.size))}</span>
    </button>
  `).join('');

  return `
    <div class="card soft logs-browser">
      <div class="logs-toolbar">
        <input id="logNameFilter" placeholder="Filter files and folders">
        <button class="secondary" id="reloadLogsButton">Reload</button>
      </div>
      <div class="muted log-home">${EscapeHtml(tree.home || '')}</div>
      <div class="log-current-path">${EscapeHtml(tree.path || '/')}</div>
      <div class="log-tree">
        ${upButton}
        ${dirsHtml}
        ${filesHtml}
      </div>
    </div>
  `;
}

function BindLogsUi(tree, selectedPath)
{
  document.querySelectorAll('.log-folder-button').forEach(button =>
  {
    button.addEventListener('click', () => RenderLogs(button.dataset.logFolder || '', ''));
  });

  document.querySelectorAll('.log-file-button').forEach(button =>
  {
    button.addEventListener('click', () => RenderLogs(tree.path, button.dataset.logFile || '', 0));
  });

  const reloadLogsButton = $('reloadLogsButton');
  if (reloadLogsButton)
    reloadLogsButton.addEventListener('click', () => RenderLogs(tree.path, selectedPath, CurrentLogState.offset));

  const reloadLogFileButton = $('reloadLogFileButton');
  if (reloadLogFileButton)
    reloadLogFileButton.addEventListener('click', () => RenderLogs(tree.path, selectedPath, CurrentLogState.offset));

  const toggleLogWrapButton = $('toggleLogWrapButton');
  if (toggleLogWrapButton)
  {
    toggleLogWrapButton.addEventListener('click', () =>
    {
      CurrentLogState.nowrap = !CurrentLogState.nowrap;
      RenderLogsLayout(tree, selectedPath);
    });
  }

  const toggleLogsLayoutButton = $('toggleLogsLayoutButton');
  if (toggleLogsLayoutButton)
  {
    toggleLogsLayoutButton.addEventListener('click', () =>
    {
      CurrentLogState.layout = CurrentLogState.layout === 'horizontal' ? 'vertical' : 'horizontal';
      RenderLogsLayout(tree, selectedPath);
    });
  }

  const downloadLogFileButton = $('downloadLogFileButton');
  if (downloadLogFileButton)
  {
    downloadLogFileButton.addEventListener('click', async () =>
    {
      downloadLogFileButton.disabled = true;
      downloadLogFileButton.textContent = 'Downloading...';
      try
      {
        await DownloadLogFile(selectedPath);
      }
      catch (e)
      {
        alert(e.message || e);
      }
      finally
      {
        downloadLogFileButton.disabled = false;
        downloadLogFileButton.textContent = 'Download';
      }
    });
  }

  const stopLogLoadingButton = $('stopLogLoadingButton');
  if (stopLogLoadingButton)
    stopLogLoadingButton.addEventListener('click', StopLogLoading);

  const firstLogChunkButton = $('firstLogChunkButton');
  if (firstLogChunkButton)
    firstLogChunkButton.addEventListener('click', () => RenderLogs(tree.path, selectedPath, 0));

  const prevLogChunkButton = $('prevLogChunkButton');
  if (prevLogChunkButton)
  {
    prevLogChunkButton.addEventListener('click', () =>
    {
      const offset = Math.max(0, CurrentLogState.offset - CurrentLogState.limit);
      RenderLogs(tree.path, selectedPath, offset);
    });
  }

  const nextLogChunkButton = $('nextLogChunkButton');
  if (nextLogChunkButton)
  {
    nextLogChunkButton.addEventListener('click', () =>
    {
      const offset = Math.min(CurrentLogState.size, CurrentLogState.offset + CurrentLogState.limit);
      RenderLogs(tree.path, selectedPath, offset);
    });
  }

  const lastLogChunkButton = $('lastLogChunkButton');
  if (lastLogChunkButton)
  {
    lastLogChunkButton.addEventListener('click', () =>
    {
      const offset = Math.max(0, CurrentLogState.size - CurrentLogState.limit);
      RenderLogs(tree.path, selectedPath, offset);
    });
  }

  const logSearchInput = $('logSearchInput');
  if (logSearchInput)
  {
    logSearchInput.addEventListener('input', () =>
    {
      CurrentLogState.search = logSearchInput.value;
      CurrentLogState.searchIndex = -1;
      RefreshLogSearch(true);
    });
  }

  const prevLogSearchButton = $('prevLogSearchButton');
  if (prevLogSearchButton)
  {
    prevLogSearchButton.addEventListener('click', () =>
    {
      const count = (CurrentLogState.searchMatches || []).length;
      if (!count)
        return;

      CurrentLogState.searchIndex = (CurrentLogState.searchIndex + count - 1) % count;
      UpdateLogSearchNavigation();
      ScrollToSearchMatch(CurrentLogState.searchIndex);
    });
  }

  const nextLogSearchButton = $('nextLogSearchButton');
  if (nextLogSearchButton)
  {
    nextLogSearchButton.addEventListener('click', () =>
    {
      const count = (CurrentLogState.searchMatches || []).length;
      if (!count)
        return;

      CurrentLogState.searchIndex = (CurrentLogState.searchIndex + 1) % count;
      UpdateLogSearchNavigation();
      ScrollToSearchMatch(CurrentLogState.searchIndex);
    });
  }

  const filterInput = $('logNameFilter');
  if (filterInput)
  {
    filterInput.addEventListener('input', () =>
    {
      const filter = filterInput.value.trim().toLowerCase();
      document.querySelectorAll('.log-tree-item').forEach(item =>
      {
        item.classList.toggle('hidden', filter && !item.textContent.toLowerCase().includes(filter));
      });
    });
  }
}

function RenderLogsLayout(tree, selectedPath = '')
{
  $('workArea').innerHTML = RenderLogViewer(tree, selectedPath);
  BindLogsUi(tree, selectedPath);
  UpdateLogLoadingStatus();
  UpdateLogSearchNavigation();
}

async function RenderLogs(path = '', selectedPath = '', offset = 0)
{
  StopLogLoading();
  $('workArea').innerHTML = '<div class="loading">Loading logs...</div>';

  const tree = await LoadLogsTree(path);

  CurrentLogState.tree = tree;
  CurrentLogState.treePath = tree.path;
  CurrentLogState.filePath = selectedPath;
  CurrentLogState.offset = Math.max(0, offset || 0);
  CurrentLogState.limit = LOG_PAGE_CHUNK_SIZE;
  CurrentLogState.size = 0;
  CurrentLogState.loadedEnd = CurrentLogState.offset;
  CurrentLogState.content = '';
  CurrentLogState.renderLevel = 'info';
  CurrentLogState.loadError = '';

  if (!selectedPath)
    CurrentLogState.search = '';

  RenderLogsLayout(tree, selectedPath);

  if (selectedPath)
    StartProgressiveLogLoad(selectedPath, CurrentLogState.offset);
}


async function SelectView(view)
{
  CurrentView = view;

  document.querySelectorAll('.nav-item').forEach(item =>
  {
    item.classList.toggle('active', item.dataset.view === view);
  });

  const titleMap = {
    overview: 'Overview',
    channels: 'Channels',
    subsystems: 'Subsystems',
    tracepoints: 'Trace points',
    logs: 'Logs',
    manual: 'Manual command'
  };

  $('viewTitle').textContent = titleMap[view] || view;

  try
  {
    if (view === 'overview')
      await RenderOverview();
    else if (view === 'channels')
      await RenderChannels();
    else if (view === 'subsystems')
      await RenderSubsystems();
    else if (view === 'tracepoints')
      await RenderTracepoints();
    else if (view === 'logs')
      await RenderLogs();
    else if (view === 'manual')
      await RenderManual();
  }
  catch (e)
  {
    $('workArea').innerHTML = `<div class="error-card">${EscapeHtml(e)}</div>`;
  }
}

function SetupEvents()
{
  $('refreshDiscoveryButton').addEventListener('click', LoadDiscovery);

  $('manualConnectButton').addEventListener('click', () =>
  {
    ConnectToTarget(
      {
        process: 'Manual target',
        control: {
          host: $('manualHost').value,
          port: Number($('manualPort').value),
          protocol: $('manualProtocol').value
        }
      },
      $('manualPassword').value
    );
  });

  $('disconnectButton').addEventListener('click', () =>
  {
    CurrentTarget = null;
    CachedChannels = null;
    SetScreen('connect');
    LoadDiscovery();
  });

  $('logoutButton').addEventListener('click', async () =>
  {
    try
    {
      await fetch('/api/logout', { method: 'POST' });
    }
    finally
    {
      window.location.href = '/login';
    }
  });

  $('reloadViewButton').addEventListener('click', () =>
  {
    CachedChannels = null;
    SelectView(CurrentView);
  });

  document.querySelectorAll('.nav-item').forEach(item =>
  {
    item.addEventListener('click', () => SelectView(item.dataset.view));
  });
}

window.addEventListener('load', () =>
{
  SetupEvents();
  LoadDiscovery();
});
