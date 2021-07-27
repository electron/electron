/**
 * Add a listener function for the connection monitor state transitions. See
 * the `addConnectionMonitorListener` documentation in `preload.js` for more
 * details. Messages transmitted from the main process to the renderer process
 * will use a action/reducer pattern. Actions have a type and a corresponding
 * payload. Access the available action types using:
 * `window.mainAPI.constants.CM_ACTION_TYPES.RENDERER`
 */
function connectionMonitorListener (_, action) {
  const { CM_ACTION_TYPES } = window.mainAPI.constants;
  switch (action.type) {
    case CM_ACTION_TYPES.RENDERER.ERROR: {
      updateError(action.payload);
      break;
    }
    case CM_ACTION_TYPES.RENDERER.TRANSITION: {
      updatePage(action.payload);
      break;
    }
    default:
      console.log(`Unrecognized action ${action.type}`);
  }
}

window.mainAPI.addConnectionMonitorListener(connectionMonitorListener);

/**
 * When the page finished loading, request the current state of the connection
 * monitor. This is important because the main process starts executing before
 * the page finished loading for the first time. Furthermore, if the renderer
 * process is reloaded on change, then this method will let the main process
 * state persist through renderer process reloads.
 */
window.onload = () => {
  window.mainAPI.requestState().then((states) => {
    updatePage(states);
  });
};

/**
 * Update the page with the `states` payload from a state transition or the
 * result of `requestState`.
 */
function updatePage (states) {
  if (states.connectionMonitor) {
    updateConnectionMonitorElements(states.connectionMonitor);
  }

  if (states.ping) {
    updatePingStatus(states.ping);
  }
}

/** Update the relative ping elements on the page */
function updatePingStatus ({ value }) {
  const { PING_STATES } = window.mainAPI.constants;
  const pingStatus = document.getElementById('ping-status');
  pingStatus.innerHTML = value === PING_STATES.TIMEOUT ? 'pinging' : value;
}

/**
 * Update the connection monitor elements on the page.
 */
function updateConnectionMonitorElements ({ value, context }) {
  const { CM_STATES } = window.mainAPI.constants;

  const connectionStatus = document.getElementById('connection-status');
  const authButton = document.getElementById('toggle-auth');
  const userDetailsContainer = document.getElementById(
    'user-details-container'
  );
  const userDetails = document.getElementById('user-details');

  connectionStatus.innerHTML = value;
  switch (value) {
    case CM_STATES.DISCONNECTED: {
      authButton.disabled = true;
      authButton.innerHTML = 'Log In';
      userDetailsContainer.hidden = true;
      break;
    }
    case CM_STATES.CONNECTED: {
      authButton.disabled = false;
      authButton.innerHTML = 'Log In';
      userDetailsContainer.hidden = true;
      break;
    }
    case CM_STATES.AUTHENTICATING: {
      authButton.disabled = true;
      authButton.innerHTML = 'Authenticating...';
      userDetailsContainer.hidden = true;
      break;
    }
    case CM_STATES.AUTHENTICATED: {
      authButton.disabled = false;
      authButton.innerHTML = 'Log Out';
      userDetailsContainer.hidden = false;
      userDetails.innerHTML = JSON.stringify(
        {
          id: context.user.id,
          username: context.user.username
        },
        0,
        2
      );
      break;
    }
    default: {
      console.error(`Unrecognized state ${value}`);
    }
  }
}

/**
 * Update the error message element
 */
function updateError ({ error }) {
  document.getElementById('error').hidden = false;
  document.getElementById('error-message').innerHTML = error.message;
}

/**
 * Adds an onlick handler for the toggle ping button. See the `togglePing`
 * documentation in `preload.js` for more details.
 */
document.getElementById('toggle-ping').onclick = function onPingButtonClick () {
  window.mainAPI.togglePing();
};

/**
 * Adds an onclick handler for the clear error button. This is only present
 * when an error is being rendered.
 */
document.getElementById('clear-error').onclick =
  function onClearErrorButtonClick () {
    document.getElementById('error-message').innerHTML = '';
    document.getElementById('error').hidden = true;
  };

document.getElementById('toggle-auth').onclick = function onAuthButtonClick () {
  window.mainAPI.toggleAuth();
};
