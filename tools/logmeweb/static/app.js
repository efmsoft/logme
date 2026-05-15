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
    host: control.host || item.host || '127.0.0.1',
    port: Number(control.port || item.port || 0),
    protocol: (control.protocol || item.protocol || 'http').toLowerCase(),
    authRequired: Boolean(item.authRequired),
    password: password || ''
  };
}

function ConnectToTarget(item, password)
{
  CurrentTarget = NormalizeTarget(item, password);
  CurrentView = 'overview';
  CachedChannels = null;
  UpdateTargetSummary();
  SetScreen('main');
  SelectView('overview');
}

function UpdateTargetSummary()
{
  const target = CurrentTarget;

  $('targetSummary').innerHTML = `
    <div class="target-name">${EscapeHtml(target.process)}</div>
    <div class="target-line">${EscapeHtml(target.protocol.toUpperCase())} ${EscapeHtml(target.host)}:${EscapeHtml(target.port)}</div>
    <div class="target-line">PID: ${EscapeHtml(target.pid || 'n/a')}</div>
    <div class="target-line">Control version: ${EscapeHtml(target.version || 'n/a')}</div>
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
        ConnectToTarget(item, password);
      });

      list.appendChild(div);
    }
  }
  catch (e)
  {
    status.textContent = `Discovery failed: ${e}`;
  }
}

async function SendCommand(command, format = 'text')
{
  if (!CurrentTarget)
    throw new Error('No target selected');

  const response = await fetch('/api/command', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      host: CurrentTarget.host,
      port: CurrentTarget.port,
      protocol: CurrentTarget.protocol,
      password: CurrentTarget.password,
      command: command,
      format: format
    })
  });

  const data = await response.json();

  if (!response.ok)
    throw new Error(data.detail || 'request failed');

  return data;
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

  if (type === 'FileBackend' || type === 'SharedFileBackend')
  {
    return `
      <label>File name</label>
      <input id="addBackendFileName" placeholder="log file path">

      <label class="check-line">
        <input id="addBackendAppend" type="checkbox" checked>
        Append to existing file
      </label>
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
    const append = $('addBackendAppend') ? $('addBackendAppend').checked : false;

    if (fileName)
      tail += ` --file ${QuoteArg(fileName)}`;

    tail += append ? ' --append' : ' --overwrite';
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

    $('enableChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --enable ${QuoteArg(channel || '<default>')}`, 'channelActionResult'));
    $('disableChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --disable ${QuoteArg(channel || '<default>')}`, 'channelActionResult'));
    $('deleteChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --delete ${QuoteArg(channel)}`, 'channelActionResult'));
    $('setErrorChannelButton').addEventListener('click', () => ExecuteChannelAction(channel, `channel --error ${QuoteArg(channel || '<default>')}`, 'channelActionResult'));
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
      ExecuteChannelAction(channel, `channel --bind ${QuoteArg(channel)} ${QuoteArg(target)}`, 'channelActionResult');
    });

    if ($('unbindChannelChipButton'))
    {
      $('unbindChannelChipButton').addEventListener('click', () =>
      {
        ExecuteChannelAction(channel, `channel --unbind ${QuoteArg(channel || '<default>')}`, 'channelActionResult');
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

      ['addBackendFileName', 'addBackendAppend'].forEach(id =>
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

  document.querySelectorAll('#channelList .list-item').forEach(button =>
  {
    button.addEventListener('click', async () =>
    {
      document.querySelectorAll('#channelList .list-item').forEach(x => x.classList.remove('selected'));
      button.classList.add('selected');
      await RenderChannelDetails(button.dataset.channel);
    });
  });
}

async function RenderSubsystems()
{
  $('workArea').innerHTML = '<div class="loading">Loading subsystems...</div>';

  const result = await SendCommand('subsystem', 'json');
  const data = result.json?.data || {};
  const blocked = data.blockedSubsystems || [];
  const allowed = data.allowedSubsystems || [];

  $('workArea').innerHTML = `
    <div class="cards-grid">
      <div class="card soft">
        <h2>Blocked subsystems</h2>
        ${RenderTags(blocked, 'No blocked subsystems')}
      </div>
      <div class="card soft">
        <h2>Allowed subsystems</h2>
        ${RenderTags(allowed, 'No allowed subsystems')}
      </div>
    </div>
    ${RenderResponseCard('Raw response', 'subsystem', result)}
  `;
}

async function RenderTracepoints()
{
  $('workArea').innerHTML = '<div class="loading">Loading trace points...</div>';

  const result = await SendCommand('trace', 'text');

  $('workArea').innerHTML = `
    <div class="toolbar">
      <input id="traceFilter" placeholder="Filter trace points...">
    </div>
    <div id="traceOutput" class="result-card">
      <div class="result-header">
        <div>
          <div class="result-title">Trace points</div>
          <div class="command-line">trace</div>
        </div>
        <span class="badge ${result.ok ? 'ok' : 'bad'}">${result.ok ? 'OK' : 'ERROR'}</span>
      </div>
      <pre>${EscapeHtml(result.text || '')}</pre>
    </div>
  `;

  $('traceFilter').addEventListener('input', () =>
  {
    const filter = $('traceFilter').value.toLowerCase();
    const lines = (result.text || '').split(/\r?\n/);
    const filtered = lines.filter(line => line.toLowerCase().includes(filter)).join('\n');
    document.querySelector('#traceOutput pre').textContent = filtered;
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
      </select>
      <div id="builderChannelTarget"></div>
    `;

    const update = () =>
    {
      const op = $('builderChannelOp').value;
      const name = op === '--create'
        ? ($('builderChannelName') ? $('builderChannelName').value : '')
        : ($('builderChannel') ? $('builderChannel').value : '');

      SetManualCommand(`channel ${op} ${QuoteArg(name)}`.trim());
    };

    const renderTarget = () =>
    {
      const op = $('builderChannelOp').value;
      const target = $('builderChannelTarget');

      if (op === '--create')
      {
        target.innerHTML = `
          <label>New channel name</label>
          <input id="builderChannelName" placeholder="channel name">
        `;
        $('builderChannelName').addEventListener('input', update);
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
    `;
    const update = () => SetManualCommand(`backend${GetBuilderChannelArg()} ${$('builderBackendOp').value} ${$('builderBackendType').value}`);
    $('builderChannel').addEventListener('change', update);
    $('builderBackendOp').addEventListener('change', update);
    $('builderBackendType').addEventListener('change', update);
    update();
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
        <option value="overview">overview</option>
          <option value="list">list</option>
        <option value="stat">stat</option>
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

        <button class="primary" id="sendManualButton">Send command</button>
      </div>

      <div id="manualResult" class="manual-result"></div>
    </div>
  `;

  $('commandPreset').addEventListener('change', UpdateManualBuilder);
  UpdateManualBuilder();

  $('sendManualButton').addEventListener('click', async () =>
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
  });
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
    manual: 'Manual command'
  };

  $('viewTitle').textContent = titleMap[view] || view;
  $('viewEyebrow').textContent = `${CurrentTarget.process} · ${CurrentTarget.host}:${CurrentTarget.port}`;

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
